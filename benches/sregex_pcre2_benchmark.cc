
#define PCRE2_CODE_UNIT_WIDTH 8

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <benchmark/benchmark.h>
#include <pcre2.h>

#include "src/automata/dfa.h"
#include "src/automata/nfa.h"

namespace {
enum class ScenarioMode {
  FullMatch,
  PartialMatch,
};

struct BenchmarkScenario {
  std::string name_;
  std::string pattern_;
  std::string input_;
  ScenarioMode mode_ = ScenarioMode::FullMatch;
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
  Sregex::Automata::Dfa dfa_;
  Pcre2Resources pcre2_;
  Pcre2Resources pcre2_jit_;
};

struct PreparedMultiScenario {
  std::string name_;
  ScenarioMode mode_ = ScenarioMode::FullMatch;
  std::vector<std::string> patterns_;
  std::vector<std::string> inputs_;
  Sregex::Automata::Nfa nfa_;
  Sregex::Automata::Dfa dfa_;
  Pcre2Resources pcre2_;
  Pcre2Resources pcre2_jit_;
  std::vector<Pcre2Resources> pcre2_per_pattern_;
  std::vector<Pcre2Resources> pcre2_jit_per_pattern_;
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

const std::vector<BenchmarkScenario>& benchmarkScenarios() {
  static const std::vector<BenchmarkScenario> scenarios = {
      {"FullMatch_LiteralLongPayload", "sregex-abcdefghijklmnopqrstuvwxyz0123456789",
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
       }(),
       ScenarioMode::FullMatch},
      {
          "FullMatch_Email",
          "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}",
          "zhouyu.sregex-team+perf42@example-bench.co.uk",
          ScenarioMode::FullMatch,
      },
      {
          "FullMatch_URL",
          "https?://([A-Za-z0-9-]{1,15}[.]){1,3}[A-Za-z]{2,8}(/"
          "[A-Za-z0-9._~:@!$&'()*+,;=-]{1,15}){1,5}",
          "https://api-v2.bench.example.com/v1/search/items-42?q=sregex&lang=en-US#top",
          ScenarioMode::FullMatch,
      },
      {
          "FullMatch_Token",
          "[A-Za-z_][A-Za-z0-9_]{2,15}",
          "sregex_BENCH_token_20261021",
          ScenarioMode::FullMatch,
      },
      {
          "FullMatch_RepeatedSegments",
          "(ab|abc|abcd){8,15}XYZ",
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "abcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcdabcd"
          "XYZ",
          ScenarioMode::FullMatch,
      },
      {
          "FullMatch_LogLine",
          "(INFO|WARN|ERROR)[A-Za-z0-9_]{3,15} [A-Za-z0-9._/-]{8,15}(/[A-Za-z0-9._-]+){1,6}",
          "WARNworker_07 services/auth/token-cache/refresh_success_2026_10_21",
          ScenarioMode::FullMatch,
      },
      {
          "PartialMatch_QuantifierPrefix",
          "a{3,5}b",
          []() {
            const size_t length = 4096;
            const std::string suffix = "aaaa";
            std::string payload(length - suffix.size(), 'a');
            payload += suffix;
            return payload;
          }(),
          ScenarioMode::PartialMatch,
      },
      {
          "PartialMatch_AlternationPrefix",
          "(cat|dog|elephant)s?",
          []() {
            const size_t length = 4096;
            const std::string suffix = "ele";
            std::string payload(length - suffix.size(), 'e');
            payload += suffix;
            return payload;
          }(),
          ScenarioMode::PartialMatch,
      },
      {
          "PartialMatch_RepeatedSegments",
          "(ab|abc|abcd){8,15}XYZ",
          []() {
            const size_t length = 4096;
            const std::string unit = "abcd";
            std::string payload;
            payload.reserve(length);
            while (payload.size() < length) {
              payload += unit;
            }
            if (payload.size() > length) {
              payload.resize(length);
            }
            return payload;
          }(),
          ScenarioMode::PartialMatch,
      },
  };

  return scenarios;
}

std::string buildAlternationPattern(const std::vector<std::string>& patterns) {
  std::string alternation;
  for (size_t i = 0; i < patterns.size(); ++i) {
    if (i > 0) {
      alternation += "|";
    }
    alternation += "(?:";
    alternation += patterns[i];
    alternation += ")";
  }
  return alternation;
}

const std::vector<PreparedScenario>& preparedScenarios() {
  static const std::vector<PreparedScenario> kPrepared = []() {
    std::vector<PreparedScenario> prepared;
    const auto& all_scenarios = benchmarkScenarios();
    prepared.reserve(all_scenarios.size());
    for (const auto& scenario : all_scenarios) {
      // Prepare the scenario for benchmarking
      PreparedScenario prepared_scenario;
      prepared_scenario.scenario_ = &scenario;
      prepared_scenario.nfa_ = std::move(Sregex::Automata::Nfa(scenario.pattern_));
      prepared_scenario.dfa_ = std::move(Sregex::Automata::Dfa(scenario.pattern_));
      if (!prepared_scenario.nfa_.ok()) {
        throw std::runtime_error("NFA build failed: " + prepared_scenario.nfa_.err());
      }
      if (!prepared_scenario.dfa_.ok()) {
        throw std::runtime_error("DFA build failed: " + prepared_scenario.dfa_.err());
      }
      prepared_scenario.pcre2_ = buildPcre2(scenario.pattern_, false);
      prepared_scenario.pcre2_jit_ = buildPcre2(scenario.pattern_, true);

      // Verify that both engines can process the benchmark input for the configured mode.
      if (scenario.mode_ == ScenarioMode::FullMatch) {
        if (!prepared_scenario.nfa_.isMatch(scenario.input_)) {
          throw std::runtime_error("NFA did not match the full-match benchmark input");
        }
        if (!prepared_scenario.dfa_.isMatch(scenario.input_)) {
          throw std::runtime_error("DFA did not match the full-match benchmark input");
        }
        const int match_result = pcre2_match_8(
            prepared_scenario.pcre2_.compiled_,
            reinterpret_cast<PCRE2_SPTR8>(scenario.input_.c_str()), scenario.input_.size(), 0, 0,
            prepared_scenario.pcre2_.match_data_, nullptr);

        if (match_result < 0 && match_result != PCRE2_ERROR_NOMATCH) {
          throw std::runtime_error("PCRE2 full-match failed");
        }

        if (prepared_scenario.pcre2_jit_.jit_ready_) {
          const int jit_match_result =
              pcre2_match_8(prepared_scenario.pcre2_jit_.compiled_,
                            reinterpret_cast<PCRE2_SPTR8>(scenario.input_.c_str()),
                            scenario.input_.size(), 0, 0, prepared_scenario.pcre2_jit_.match_data_,
                            prepared_scenario.pcre2_jit_.match_context_);

          if (jit_match_result < 0 && jit_match_result != PCRE2_ERROR_NOMATCH) {
            throw std::runtime_error("PCRE2 JIT full-match failed");
          }
        }
      } else {
        if (!prepared_scenario.nfa_.isPartialMatch(scenario.input_)) {
          throw std::runtime_error("NFA did not detect partial match for benchmark input: " +
                                   scenario.name_);
        }
        if (!prepared_scenario.dfa_.isPartialMatch(scenario.input_)) {
          throw std::runtime_error("DFA did not detect partial match for benchmark input: " +
                                   scenario.name_);
        }

        size_t match_length =
            std::min(prepared_scenario.nfa_.longestPath(), scenario.input_.size());
        const char* match_start = scenario.input_.c_str() + (scenario.input_.size() - match_length);

        const uint32_t partial_flags = PCRE2_PARTIAL_SOFT;
        const int partial_result = pcre2_match_8(
            prepared_scenario.pcre2_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
            match_length, 0, partial_flags, prepared_scenario.pcre2_.match_data_, nullptr);
        if (partial_result < 0 && partial_result != PCRE2_ERROR_NOMATCH &&
            partial_result != PCRE2_ERROR_PARTIAL) {
          throw std::runtime_error("PCRE2 partial-match failed");
        }

        if (prepared_scenario.pcre2_jit_.jit_ready_) {
          const int jit_partial_result = pcre2_match_8(
              prepared_scenario.pcre2_jit_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
              match_length, 0, partial_flags, prepared_scenario.pcre2_jit_.match_data_,
              prepared_scenario.pcre2_jit_.match_context_);
          if (jit_partial_result < 0 && jit_partial_result != PCRE2_ERROR_NOMATCH &&
              jit_partial_result != PCRE2_ERROR_PARTIAL) {
            throw std::runtime_error("PCRE2 JIT partial-match failed");
          }
        }
      }

      prepared.push_back(std::move(prepared_scenario));
    }
    return prepared;
  }();

  return kPrepared;
}

const std::vector<PreparedMultiScenario>& preparedMultiScenarios() {
  static const std::vector<PreparedMultiScenario> kPrepared = []() {
    std::vector<std::string> full_patterns;
    std::vector<std::string> full_inputs;
    std::vector<std::string> partial_patterns;
    std::vector<std::string> partial_inputs;

    for (const auto& scenario : benchmarkScenarios()) {
      if (scenario.mode_ == ScenarioMode::FullMatch) {
        full_patterns.push_back(scenario.pattern_);
        full_inputs.push_back(scenario.input_);
      } else {
        partial_patterns.push_back(scenario.pattern_);
        partial_inputs.push_back(scenario.input_);
      }
    }

    std::vector<PreparedMultiScenario> prepared;

    if (!full_patterns.empty()) {
      PreparedMultiScenario full_multi;
      full_multi.name_ = "MultiPattern_FullMatch";
      full_multi.mode_ = ScenarioMode::FullMatch;
      full_multi.patterns_ = full_patterns;
      full_multi.inputs_ = full_inputs;
      full_multi.nfa_ = Sregex::Automata::Nfa(std::span<const std::string>(full_patterns));
      full_multi.dfa_ = Sregex::Automata::Dfa(std::span<const std::string>(full_patterns));
      if (!full_multi.nfa_.ok()) {
        throw std::runtime_error("NFA multi-pattern build failed: " + full_multi.nfa_.err());
      }
      if (!full_multi.dfa_.ok()) {
        throw std::runtime_error("DFA multi-pattern build failed: " + full_multi.dfa_.err());
      }

      const std::string full_alternation = buildAlternationPattern(full_patterns);
      full_multi.pcre2_ = buildPcre2(full_alternation, false);
      full_multi.pcre2_jit_ = buildPcre2(full_alternation, true);
      full_multi.pcre2_per_pattern_.reserve(full_patterns.size());
      full_multi.pcre2_jit_per_pattern_.reserve(full_patterns.size());
      for (const auto& pattern : full_patterns) {
        full_multi.pcre2_per_pattern_.push_back(buildPcre2(pattern, false));
        full_multi.pcre2_jit_per_pattern_.push_back(buildPcre2(pattern, true));
      }

      for (const auto& input : full_multi.inputs_) {
        if (!full_multi.nfa_.isMatch(input)) {
          throw std::runtime_error("NFA multi-pattern full-match verification failed");
        }
        if (!full_multi.dfa_.isMatch(input.c_str(), input.size())) {
          throw std::runtime_error("DFA multi-pattern full-match verification failed");
        }

        const int match_result =
            pcre2_match_8(full_multi.pcre2_.compiled_, reinterpret_cast<PCRE2_SPTR8>(input.c_str()),
                          input.size(), 0, 0, full_multi.pcre2_.match_data_, nullptr);
        if (match_result <= 0) {
          throw std::runtime_error("PCRE2 multi-pattern full-match verification failed");
        }

        if (full_multi.pcre2_jit_.jit_ready_) {
          const int jit_match_result = pcre2_match_8(
              full_multi.pcre2_jit_.compiled_, reinterpret_cast<PCRE2_SPTR8>(input.c_str()),
              input.size(), 0, 0, full_multi.pcre2_jit_.match_data_,
              full_multi.pcre2_jit_.match_context_);
          if (jit_match_result <= 0) {
            throw std::runtime_error("PCRE2 JIT multi-pattern full-match verification failed");
          }
        }
      }

      prepared.push_back(std::move(full_multi));
    }

    if (!partial_patterns.empty()) {
      PreparedMultiScenario partial_multi;
      partial_multi.name_ = "MultiPattern_PartialMatch";
      partial_multi.mode_ = ScenarioMode::PartialMatch;
      partial_multi.patterns_ = partial_patterns;
      partial_multi.inputs_ = partial_inputs;
      partial_multi.nfa_ = Sregex::Automata::Nfa(std::span<const std::string>(partial_patterns));
      partial_multi.dfa_ = Sregex::Automata::Dfa(std::span<const std::string>(partial_patterns));
      if (!partial_multi.nfa_.ok()) {
        throw std::runtime_error("NFA multi-pattern partial build failed: " +
                                 partial_multi.nfa_.err());
      }
      if (!partial_multi.dfa_.ok()) {
        throw std::runtime_error("DFA multi-pattern partial build failed");
      }

      const std::string partial_alternation = buildAlternationPattern(partial_patterns);
      partial_multi.pcre2_ = buildPcre2(partial_alternation, false);
      partial_multi.pcre2_jit_ = buildPcre2(partial_alternation, true);
      partial_multi.pcre2_per_pattern_.reserve(partial_patterns.size());
      partial_multi.pcre2_jit_per_pattern_.reserve(partial_patterns.size());
      for (const auto& pattern : partial_patterns) {
        partial_multi.pcre2_per_pattern_.push_back(buildPcre2(pattern, false));
        partial_multi.pcre2_jit_per_pattern_.push_back(buildPcre2(pattern, true));
      }

      const uint32_t partial_flags = PCRE2_PARTIAL_SOFT;
      for (const auto& input : partial_multi.inputs_) {
        if (!partial_multi.nfa_.isPartialMatch(input)) {
          throw std::runtime_error("NFA multi-pattern partial verification failed");
        }
        if (!partial_multi.dfa_.isPartialMatch(input)) {
          throw std::runtime_error("DFA multi-pattern partial verification failed");
        }

        const size_t match_length = std::min(partial_multi.nfa_.longestPath(), input.size());
        const char* match_start = input.c_str() + (input.size() - match_length);

        const int partial_result = pcre2_match_8(
            partial_multi.pcre2_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
            match_length, 0, partial_flags, partial_multi.pcre2_.match_data_, nullptr);
        if (partial_result < 0 && partial_result != PCRE2_ERROR_NOMATCH &&
            partial_result != PCRE2_ERROR_PARTIAL) {
          throw std::runtime_error("PCRE2 multi-pattern partial verification failed");
        }

        if (partial_multi.pcre2_jit_.jit_ready_) {
          const int jit_partial_result = pcre2_match_8(
              partial_multi.pcre2_jit_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
              match_length, 0, partial_flags, partial_multi.pcre2_jit_.match_data_,
              partial_multi.pcre2_jit_.match_context_);
          if (jit_partial_result < 0 && jit_partial_result != PCRE2_ERROR_NOMATCH &&
              jit_partial_result != PCRE2_ERROR_PARTIAL) {
            throw std::runtime_error("PCRE2 JIT multi-pattern partial verification failed");
          }
        }
      }

      prepared.push_back(std::move(partial_multi));
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

static void BM_Sregex_Dfa_Scenario(benchmark::State& state, const PreparedScenario& prepared) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(prepared.dfa_.isMatch(prepared.scenario_->input_.c_str(),
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

static void BM_Sregex_Nfa_PartialScenario(benchmark::State& state,
                                          const PreparedScenario& prepared) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(prepared.nfa_.isPartialMatch(prepared.scenario_->input_.c_str(),
                                                          prepared.scenario_->input_.size()));
  }
}

static void BM_Sregex_Dfa_PartialScenario(benchmark::State& state,
                                          const PreparedScenario& prepared) {
  for (auto _ : state) {
    benchmark::DoNotOptimize(prepared.dfa_.isPartialMatch(prepared.scenario_->input_.c_str(),
                                                          prepared.scenario_->input_.size()));
  }
}

static void BM_PCRE2_PartialScenario(benchmark::State& state, const PreparedScenario& prepared) {
  size_t match_length = std::min(prepared.nfa_.longestPath(), prepared.scenario_->input_.size());
  const char* match_start =
      prepared.scenario_->input_.c_str() + (prepared.scenario_->input_.size() - match_length);

  for (auto _ : state) {
    int match_result =
        pcre2_match_8(prepared.pcre2_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
                      match_length, 0, PCRE2_PARTIAL_SOFT, prepared.pcre2_.match_data_, nullptr);
    benchmark::DoNotOptimize(match_result);
  }
}

static void BM_PCRE2_JIT_PartialScenario(benchmark::State& state,
                                         const PreparedScenario& prepared) {
  if (!prepared.pcre2_jit_.jit_ready_) {
    state.SkipWithError(prepared.pcre2_jit_.jit_error_.c_str());
    return;
  }

  size_t match_length = std::min(prepared.nfa_.longestPath(), prepared.scenario_->input_.size());
  const char* match_start =
      prepared.scenario_->input_.c_str() + (prepared.scenario_->input_.size() - match_length);

  for (auto _ : state) {
    int match_result = pcre2_match_8(
        prepared.pcre2_jit_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start), match_length, 0,
        PCRE2_PARTIAL_SOFT, prepared.pcre2_jit_.match_data_, prepared.pcre2_jit_.match_context_);
    benchmark::DoNotOptimize(match_result);
  }
}

static void BM_Sregex_Nfa_MultiScenario(benchmark::State& state,
                                        const PreparedMultiScenario& prepared) {
  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const bool matched = (prepared.mode_ == ScenarioMode::FullMatch)
                             ? prepared.nfa_.isMatch(input.c_str(), input.size())
                             : prepared.nfa_.isPartialMatch(input.c_str(), input.size());
    benchmark::DoNotOptimize(static_cast<int>(matched));
    ++input_index;
  }
}

static void BM_Sregex_Dfa_MultiScenario(benchmark::State& state,
                                        const PreparedMultiScenario& prepared) {
  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const bool matched = (prepared.mode_ == ScenarioMode::FullMatch)
                             ? prepared.dfa_.isMatch(input.c_str(), input.size())
                             : prepared.dfa_.isPartialMatch(input.c_str(), input.size());
    benchmark::DoNotOptimize(static_cast<int>(matched));
    ++input_index;
  }
}

static void BM_PCRE2_MultiScenario(benchmark::State& state, const PreparedMultiScenario& prepared) {
  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const uint32_t partial_flags =
        (prepared.mode_ == ScenarioMode::PartialMatch) ? PCRE2_PARTIAL_SOFT : 0;
    size_t match_length = input.size();
    const char* match_start = input.c_str();

    if (prepared.mode_ == ScenarioMode::PartialMatch) {
      match_length = std::min(prepared.nfa_.longestPath(), input.size());
      match_start = input.c_str() + (input.size() - match_length);
    }

    int match_result =
        pcre2_match_8(prepared.pcre2_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
                      match_length, 0, partial_flags, prepared.pcre2_.match_data_, nullptr);
    benchmark::DoNotOptimize(match_result);
    ++input_index;
  }
}

static void BM_PCRE2_JIT_MultiScenario(benchmark::State& state,
                                       const PreparedMultiScenario& prepared) {
  if (!prepared.pcre2_jit_.jit_ready_) {
    state.SkipWithError(prepared.pcre2_jit_.jit_error_.c_str());
    return;
  }

  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const uint32_t partial_flags =
        (prepared.mode_ == ScenarioMode::PartialMatch) ? PCRE2_PARTIAL_SOFT : 0;
    size_t match_length = input.size();
    const char* match_start = input.c_str();

    if (prepared.mode_ == ScenarioMode::PartialMatch) {
      match_length = std::min(prepared.nfa_.longestPath(), input.size());
      match_start = input.c_str() + (input.size() - match_length);
    }

    int match_result = pcre2_match_8(
        prepared.pcre2_jit_.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start), match_length, 0,
        partial_flags, prepared.pcre2_jit_.match_data_, prepared.pcre2_jit_.match_context_);
    benchmark::DoNotOptimize(match_result);
    ++input_index;
  }
}

static void BM_PCRE2_MultiScenario_PerPatternLoop(benchmark::State& state,
                                                  const PreparedMultiScenario& prepared) {
  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const uint32_t partial_flags =
        (prepared.mode_ == ScenarioMode::PartialMatch) ? PCRE2_PARTIAL_SOFT : 0;
    size_t match_length = input.size();
    const char* match_start = input.c_str();

    if (prepared.mode_ == ScenarioMode::PartialMatch) {
      match_length = std::min(prepared.nfa_.longestPath(), input.size());
      match_start = input.c_str() + (input.size() - match_length);
    }

    int final_result = PCRE2_ERROR_NOMATCH;
    for (auto& per_pattern : prepared.pcre2_per_pattern_) {
      const int match_result =
          pcre2_match_8(per_pattern.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start),
                        match_length, 0, partial_flags, per_pattern.match_data_, nullptr);
      final_result = match_result;
      if (prepared.mode_ == ScenarioMode::FullMatch) {
        if (match_result > 0) {
          break;
        }
      } else {
        if (match_result == PCRE2_ERROR_PARTIAL || match_result > 0) {
          break;
        }
      }
    }

    benchmark::DoNotOptimize(final_result);
    ++input_index;
  }
}

static void BM_PCRE2_JIT_MultiScenario_PerPatternLoop(benchmark::State& state,
                                                      const PreparedMultiScenario& prepared) {
  size_t input_index = 0;
  for (auto _ : state) {
    const auto& input = prepared.inputs_[input_index % prepared.inputs_.size()];
    const uint32_t partial_flags =
        (prepared.mode_ == ScenarioMode::PartialMatch) ? PCRE2_PARTIAL_SOFT : 0;
    size_t match_length = input.size();
    const char* match_start = input.c_str();

    if (prepared.mode_ == ScenarioMode::PartialMatch) {
      match_length = std::min(prepared.nfa_.longestPath(), input.size());
      match_start = input.c_str() + (input.size() - match_length);
    }

    int final_result = PCRE2_ERROR_NOMATCH;
    for (size_t i = 0; i < prepared.pcre2_jit_per_pattern_.size(); ++i) {
      auto& per_pattern_jit = prepared.pcre2_jit_per_pattern_[i];
      if (!per_pattern_jit.jit_ready_) {
        continue;
      }

      const int match_result = pcre2_match_8(
          per_pattern_jit.compiled_, reinterpret_cast<PCRE2_SPTR8>(match_start), match_length, 0,
          partial_flags, per_pattern_jit.match_data_, per_pattern_jit.match_context_);
      final_result = match_result;
      if (prepared.mode_ == ScenarioMode::FullMatch) {
        if (match_result > 0) {
          break;
        }
      } else {
        if (match_result == PCRE2_ERROR_PARTIAL || match_result > 0) {
          break;
        }
      }
    }

    benchmark::DoNotOptimize(final_result);
    ++input_index;
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
      if (line_count_ <= 36) {
        if (line_count_ % 4 == 0) {
          std::cout << "-----------------------" << std::endl;
        }
      } else {
        if ((line_count_ - 36) % 6 == 0) {
          std::cout << "-----------------------" << std::endl;
        }
      }
    }
  }

private:
  size_t line_count_ = 0;
};

int main(int argc, char** argv) {
  benchmark::Initialize(&argc, argv);

  for (const auto& prepared : preparedScenarios()) {
    if (prepared.scenario_->mode_ == ScenarioMode::FullMatch) {
      benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/Sregex_NFA").c_str(),
                                   BM_Sregex_Nfa_Scenario, std::cref(prepared));
      benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/Sregex_DFA").c_str(),
                                   BM_Sregex_Dfa_Scenario, std::cref(prepared));
      benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2").c_str(),
                                   BM_PCRE2_Scenario, std::cref(prepared));
      benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2_JIT").c_str(),
                                   BM_PCRE2_JIT_Scenario, std::cref(prepared));
      continue;
    }

    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/Sregex_NFA").c_str(),
                                 BM_Sregex_Nfa_PartialScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/Sregex_DFA").c_str(),
                                 BM_Sregex_Dfa_PartialScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2").c_str(),
                                 BM_PCRE2_PartialScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.scenario_->name_ + "/PCRE2_JIT").c_str(),
                                 BM_PCRE2_JIT_PartialScenario, std::cref(prepared));
  }

  for (const auto& prepared : preparedMultiScenarios()) {
    benchmark::RegisterBenchmark((prepared.name_ + "/Sregex_NFA").c_str(),
                                 BM_Sregex_Nfa_MultiScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.name_ + "/Sregex_DFA").c_str(),
                                 BM_Sregex_Dfa_MultiScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.name_ + "/PCRE2_ALTERNATION").c_str(),
                                 BM_PCRE2_MultiScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.name_ + "/PCRE2_PER_PATTERN").c_str(),
                                 BM_PCRE2_MultiScenario_PerPatternLoop, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.name_ + "/PCRE2_JIT_ALTERNATION").c_str(),
                                 BM_PCRE2_JIT_MultiScenario, std::cref(prepared));
    benchmark::RegisterBenchmark((prepared.name_ + "/PCRE2_JIT_PER_PATTERN").c_str(),
                                 BM_PCRE2_JIT_MultiScenario_PerPatternLoop, std::cref(prepared));
  }

  GroupedConsoleReporter reporter;
  benchmark::RunSpecifiedBenchmarks(&reporter);
  benchmark::Shutdown();
  return 0;
}
