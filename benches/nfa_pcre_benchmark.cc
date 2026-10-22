
#define PCRE2_CODE_UNIT_WIDTH 8

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>
#include <pcre2.h>

#include "src/automata/nfa.h"

namespace {
struct BenchmarkScenario {
  std::string name_;
  std::string pattern_;
  std::string input_;
};

struct Pcre2Resources {
  pcre2_code_8* compiled_ = nullptr;
  pcre2_match_data_8* match_data_ = nullptr;
  pcre2_match_context_8* match_context_ = nullptr;
  pcre2_jit_stack_8* jit_stack_ = nullptr;
  bool jit_ready_ = false;
  std::string jit_error_;

  ~Pcre2Resources() {
    if (match_context_ != nullptr) {
      pcre2_match_context_free_8(match_context_);
    }
    if (jit_stack_ != nullptr) {
      pcre2_jit_stack_free_8(jit_stack_);
    }
    if (match_data_ != nullptr) {
      pcre2_match_data_free_8(match_data_);
    }
    if (compiled_ != nullptr) {
      pcre2_code_free_8(compiled_);
    }
  }

  Pcre2Resources() = default;
  Pcre2Resources(const Pcre2Resources&) = delete;
  Pcre2Resources& operator=(const Pcre2Resources&) = delete;

  Pcre2Resources(Pcre2Resources&& other) noexcept {
    compiled_ = std::exchange(other.compiled_, nullptr);
    match_data_ = std::exchange(other.match_data_, nullptr);
    match_context_ = std::exchange(other.match_context_, nullptr);
    jit_stack_ = std::exchange(other.jit_stack_, nullptr);
    jit_ready_ = other.jit_ready_;
    jit_error_ = std::move(other.jit_error_);
  }

  Pcre2Resources& operator=(Pcre2Resources&& other) noexcept {
    if (this == &other) {
      return *this;
    }
    if (match_data_ != nullptr) {
      pcre2_match_data_free_8(match_data_);
    }
    if (match_context_ != nullptr) {
      pcre2_match_context_free_8(match_context_);
    }
    if (jit_stack_ != nullptr) {
      pcre2_jit_stack_free_8(jit_stack_);
    }
    if (compiled_ != nullptr) {
      pcre2_code_free_8(compiled_);
    }
    compiled_ = std::exchange(other.compiled_, nullptr);
    match_data_ = std::exchange(other.match_data_, nullptr);
    match_context_ = std::exchange(other.match_context_, nullptr);
    jit_stack_ = std::exchange(other.jit_stack_, nullptr);
    jit_ready_ = other.jit_ready_;
    jit_error_ = std::move(other.jit_error_);
    return *this;
  }
};

struct PreparedScenario {
  const BenchmarkScenario* scenario_;
  Sregex::Automata::Nfa nfa_;
  Pcre2Resources pcre2_;
  Pcre2Resources pcre2_jit_;
};

Pcre2Resources buildPcre2(const std::string& pattern, bool enable_jit) {
  int error_code = 0;
  PCRE2_SIZE error_offset = 0;
  const auto* pattern_ptr = reinterpret_cast<PCRE2_SPTR8>(pattern.c_str());
  auto* compiled =
      pcre2_compile_8(pattern_ptr, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, nullptr);
  if (compiled == nullptr) {
    throw std::runtime_error("PCRE2 compilation failed");
  }

  auto* match_data = pcre2_match_data_create_from_pattern_8(compiled, nullptr);
  if (match_data == nullptr) {
    pcre2_code_free_8(compiled);
    throw std::runtime_error("PCRE2 match data allocation failed");
  }

  Pcre2Resources resources;
  resources.compiled_ = compiled;
  resources.match_data_ = match_data;

  if (enable_jit) {
    const int jit_rc = pcre2_jit_compile_8(resources.compiled_, PCRE2_JIT_COMPLETE);
    if (jit_rc != 0) {
      resources.jit_ready_ = false;
      resources.jit_error_ =
          "PCRE2 JIT compile failed or unsupported, code=" + std::to_string(jit_rc);
      return resources;
    }

    resources.match_context_ = pcre2_match_context_create_8(nullptr);
    if (resources.match_context_ == nullptr) {
      resources.jit_ready_ = false;
      resources.jit_error_ = "PCRE2 JIT match context allocation failed";
      return resources;
    }

    resources.jit_stack_ = pcre2_jit_stack_create_8(32 * 1024, 512 * 1024, nullptr);
    if (resources.jit_stack_ == nullptr) {
      resources.jit_ready_ = false;
      resources.jit_error_ = "PCRE2 JIT stack allocation failed";
      return resources;
    }

    pcre2_jit_stack_assign_8(resources.match_context_, nullptr, resources.jit_stack_);
    resources.jit_ready_ = true;
  }

  return resources;
}

const std::vector<PreparedScenario>& preparedScenarios() {
  static const std::vector<BenchmarkScenario> scenarios = {
      {"LiteralLongPayload", "sregex-abcdefghijklmnopqrstuvwxyz0123456789",
       []() {
         const std::string literal = "sregex-abcdefghijklmnopqrstuvwxyz0123456789";
         const size_t length = 4096;
         std::string payload;
         payload.reserve(length + literal.size());

         while (payload.size() < length) {
           payload += literal;
         }

         if (payload.size() > length) {
           payload.resize(length);
         }

         return payload;
       }()},
      {
          "Email",
          "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}",
          "zhouyu.sregex-team+perf42@example-bench.co.uk",
      },
      {
          "URL",
          "https?://([A-Za-z0-9-]{1,15}[.]){1,3}[A-Za-z]{2,8}(/"
          "[A-Za-z0-9._~:@!$&'()*+,;=-]{1,15}){1,5}",
          "https://api-v2.bench.example.com/v1/search/items-42?q=sregex&lang=en-US#top",
      },
      {
          "Token",
          "[A-Za-z_][A-Za-z0-9_]{2,15}",
          "sregex_BENCH_token_20261021",
      },
      {
          "RepeatedSegments",
          "(ab|abc|abcd){8,15}XYZ",
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "XYZ",
      },
      {
          "LogLine",
          "(INFO|WARN|ERROR)[A-Za-z0-9_]{3,15} [A-Za-z0-9._/-]{8,15}(/[A-Za-z0-9._-]+){1,6}",
          "WARNworker_07 services/auth/token-cache/refresh_success_2026_10_21",
      },
  };

  static const std::vector<PreparedScenario> kPrepared = []() {
    std::vector<PreparedScenario> prepared;
    prepared.reserve(scenarios.size());
    for (const auto& scenario : scenarios) {
      // Prepare the scenario for benchmarking
      PreparedScenario prepared_scenario;
      prepared_scenario.scenario_ = &scenario;
      prepared_scenario.nfa_ = std::move(Sregex::Automata::Nfa(scenario.pattern_));
      if (!prepared_scenario.nfa_.ok()) {
        throw std::runtime_error("NFA build failed: " + prepared_scenario.nfa_.err());
      }
      prepared_scenario.pcre2_ = buildPcre2(scenario.pattern_, false);
      prepared_scenario.pcre2_jit_ = buildPcre2(scenario.pattern_, true);

      // Verify that both NFA and PCRE2 match the input before benchmarking
      if (!prepared_scenario.nfa_.isMatch(scenario.input_)) {
        throw std::runtime_error("NFA did not match the benchmark input");
      }
      const int match_result = pcre2_match_8(prepared_scenario.pcre2_.compiled_,
                                             reinterpret_cast<PCRE2_SPTR8>(scenario.input_.c_str()),
                                             scenario.input_.size(), 0, 0,
                                             prepared_scenario.pcre2_.match_data_, nullptr);

      if (match_result < 0 && match_result != PCRE2_ERROR_NOMATCH) {
        throw std::runtime_error("PCRE2 match failed");
      }

      if (prepared_scenario.pcre2_jit_.jit_ready_) {
        const int jit_match_result = pcre2_match_8(
            prepared_scenario.pcre2_jit_.compiled_,
            reinterpret_cast<PCRE2_SPTR8>(scenario.input_.c_str()), scenario.input_.size(), 0, 0,
            prepared_scenario.pcre2_jit_.match_data_, prepared_scenario.pcre2_jit_.match_context_);

        if (jit_match_result < 0 && jit_match_result != PCRE2_ERROR_NOMATCH) {
          throw std::runtime_error("PCRE2 JIT match failed");
        }
      }

      prepared.push_back(std::move(prepared_scenario));
    }
    return prepared;
  }();

  return kPrepared;
}

} // namespace

static void BM_Sregex_Nfa_Scenario(benchmark::State& state, const PreparedScenario& prepared) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(prepared.nfa_.isMatch(prepared.scenario_->input_.c_str(),
                                                   prepared.scenario_->input_.size()));
  }
}

static void BM_PCRE2_Scenario(benchmark::State& state, const PreparedScenario& prepared) {
  for (auto _ : state) {
    int match_result = pcre2_match_8(
        prepared.pcre2_.compiled_,
        reinterpret_cast<PCRE2_SPTR8>(prepared.scenario_->input_.c_str()),
        prepared.scenario_->input_.size(), 0, 0, prepared.pcre2_.match_data_, nullptr);
    benchmark::DoNotOptimize(match_result);
  }
}

static void BM_PCRE2_JIT_Scenario(benchmark::State& state, const PreparedScenario& prepared) {
  if (!prepared.pcre2_jit_.jit_ready_) {
    state.SkipWithError(prepared.pcre2_jit_.jit_error_.c_str());
    return;
  }

  for (auto _ : state) {
    int match_result =
        pcre2_match_8(prepared.pcre2_jit_.compiled_,
                      reinterpret_cast<PCRE2_SPTR8>(prepared.scenario_->input_.c_str()),
                      prepared.scenario_->input_.size(), 0, 0, prepared.pcre2_jit_.match_data_,
                      prepared.pcre2_jit_.match_context_);
    benchmark::DoNotOptimize(match_result);
  }
}

// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, literal, preparedScenarios()[0]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, literal, preparedScenarios()[0]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, literal, preparedScenarios()[0]);
// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, email, preparedScenarios()[1]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, email, preparedScenarios()[1]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, email, preparedScenarios()[1]);
// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, url, preparedScenarios()[2]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, url, preparedScenarios()[2]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, url, preparedScenarios()[2]);
// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, token, preparedScenarios()[3]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, token, preparedScenarios()[3]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, token, preparedScenarios()[3]);
// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, repeated_segments, preparedScenarios()[4]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, repeated_segments, preparedScenarios()[4]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, repeated_segments, preparedScenarios()[4]);
// BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, log_line, preparedScenarios()[5]);
// BENCHMARK_CAPTURE(BM_PCRE2_Scenario, log_line, preparedScenarios()[5]);
// BENCHMARK_CAPTURE(BM_PCRE2_JIT_Scenario, log_line, preparedScenarios()[5]);
// BENCHMARK_MAIN();

class GroupedConsoleReporter : public benchmark::ConsoleReporter {
public:
  void ReportRuns(const std::vector<Run>& reports) override {
    benchmark::ConsoleReporter::ReportRuns(reports);

    for (const auto& run : reports) {
      if (!run.aggregate_name.empty()) {
        continue;
      }
      ++line_count_;
      if (line_count_ % 3 == 0) {
        std::cout << "-----------------------" << std::endl;
      }
    }
  }

private:
  size_t line_count_ = 0;
};

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  for (const auto& prepared : preparedScenarios()) {
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/Sregex_NFA").c_str(),
                                 BM_Sregex_Nfa_Scenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2").c_str(), BM_PCRE2_Scenario,
                                 std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2_JIT").c_str(),
                                 BM_PCRE2_JIT_Scenario, std::cref(prepared));
  }

  GroupedConsoleReporter reporter;
  benchmark::RunSpecifiedBenchmarks(&reporter);
  benchmark::Shutdown();
  return 0;
}
