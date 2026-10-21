
#define PCRE2_CODE_UNIT_WIDTH 8

#include <algorithm>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>
#include <pcre2.h>

#include "src/automata/nfa.h"

namespace {

std::string buildLiteralPayload(size_t length) {
  const std::string literal = "sregex-abcdefghijklmnopqrstuvwxyz0123456789";
  std::string payload;
  payload.reserve(length);

  while (payload.size() < length) {
    payload += literal;
  }

  if (payload.size() > length) {
    payload.resize(length);
  }

  return payload;
}

struct BenchmarkScenario {
  std::string name;
  std::string pattern;
  std::string input;
};

const std::vector<BenchmarkScenario>& scenarios() {
  static const std::vector<BenchmarkScenario> kScenarios = {
      {
          "LiteralLongPayload",
          "sregex-abcdefghijklmnopqrstuvwxyz0123456789",
          buildLiteralPayload(4096),
      },
      {
          "Email",
          "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}",
          "zhouyu.sregex-team+perf42@example-bench.co.uk",
      },
      {
          "URL",
          "https?://([A-Za-z0-9-]{1,15}[.]){1,3}[A-Za-z]{2,8}(/[A-Za-z0-9._~:@!$&'()*+,;=-]{1,15}){0,5}",
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
          "(INFO|WARN|ERROR)[[][A-Za-z0-9_]{3,15}[]] [A-Za-z0-9._/-]{8,15}(/[A-Za-z0-9._-]{1,15}){1,6}",
          "WARN[worker_07] services/auth/token-cache/refresh_success_2026_10_21",
      },
  };
  return kScenarios;
}

void runPcre2Match(const std::string& pattern, const std::string& input) {
  int error_code = 0;
  PCRE2_SIZE error_offset = 0;
  const auto* pattern_ptr = reinterpret_cast<PCRE2_SPTR8>(pattern.c_str());
  auto* compiled =
      pcre2_compile_8(pattern_ptr, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, nullptr);
  if (compiled == nullptr) {
    throw std::runtime_error("PCRE2 compilation failed");
  }

  auto* match_data = pcre2_match_data_create_from_pattern_8(compiled, nullptr);
  const int match_result = pcre2_match_8(compiled, reinterpret_cast<PCRE2_SPTR8>(input.c_str()),
                                         input.size(), 0, 0, match_data, nullptr);

  if (match_result < 0 && match_result != PCRE2_ERROR_NOMATCH) {
    pcre2_match_data_free_8(match_data);
    pcre2_code_free_8(compiled);
    throw std::runtime_error("PCRE2 match failed");
  }

  pcre2_match_data_free_8(match_data);
  pcre2_code_free_8(compiled);
}

void runNfaMatch(const std::string& pattern, const std::string& input) {
  Sregex::Automata::Nfa nfa(pattern);
  if (!nfa.ok()) {
    throw std::runtime_error("NFA build failed: " + nfa.err());
  }

  const bool matched = nfa.isMatch(input);
  if (!matched) {
    throw std::runtime_error("NFA did not match the benchmark input");
  }
}

void validateScenario(benchmark::State& state, const BenchmarkScenario& scenario) {
  try {
    runNfaMatch(scenario.pattern, scenario.input);
    runPcre2Match(scenario.pattern, scenario.input);
  } catch (const std::exception& ex) {
    state.SkipWithError(ex.what());
  }
}

} // namespace

static void BM_Sregex_Nfa_Scenario(benchmark::State& state, const BenchmarkScenario& scenario) {
  validateScenario(state, scenario);

  Sregex::Automata::Nfa nfa(scenario.pattern);
  if (!nfa.ok()) {
    state.SkipWithError(nfa.err().c_str());
  }

  for (auto _ : state) {
    benchmark::DoNotOptimize(nfa.isMatch(scenario.input));
  }
}

static void BM_PCRE2_Scenario(benchmark::State& state, const BenchmarkScenario& scenario) {
  validateScenario(state, scenario);

  int error_code = 0;
  PCRE2_SIZE error_offset = 0;
  const auto* pattern_ptr = reinterpret_cast<PCRE2_SPTR8>(scenario.pattern.c_str());
  auto* compiled =
      pcre2_compile_8(pattern_ptr, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, nullptr);
  if (compiled == nullptr) {
    state.SkipWithError("PCRE2 pattern compilation failed");
  }

  auto* match_data = pcre2_match_data_create_from_pattern_8(compiled, nullptr);

  for (auto _ : state) {
    int match_result = pcre2_match_8(compiled, reinterpret_cast<PCRE2_SPTR8>(scenario.input.c_str()),
                                     scenario.input.size(), 0, 0, match_data, nullptr);
    benchmark::DoNotOptimize(match_result);
  }

  pcre2_match_data_free_8(match_data);
  pcre2_code_free_8(compiled);
}

BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, literal, scenarios()[0]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, literal, scenarios()[0]);
BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, email, scenarios()[1]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, email, scenarios()[1]);
BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, url, scenarios()[2]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, url, scenarios()[2]);
BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, token, scenarios()[3]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, token, scenarios()[3]);
BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, repeated_segments, scenarios()[4]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, repeated_segments, scenarios()[4]);
BENCHMARK_CAPTURE(BM_Sregex_Nfa_Scenario, log_line, scenarios()[5]);
BENCHMARK_CAPTURE(BM_PCRE2_Scenario, log_line, scenarios()[5]);

BENCHMARK_MAIN();
