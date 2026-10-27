/**
 * Copyright (c) 2026 Stone Rhino and contributors.
 *
 * MIT License (http://opensource.org/licenses/MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <iostream>
#include <regex>
#include <span>
#include <string>
#include <vector>

#include "src/automata/dfa.h"

namespace {
// These patterns represent data that should be redacted in logs
const static std::vector<std::string> patterns = {
    "token_[A-Za-z0-9]{6}",
    "user_id=[0-9]{4}",
};

void processStableChunk(std::string&& stable_chunk) {
  std::cout << "[stable-chunk] Original stream: " << stable_chunk << std::endl;

  // The design intention of Sregex is to only provide the ability to determine partial matches,
  // and not to provide match positions. Therefore, for actual redaction we need to implement the
  // logic itself (using hyperscan, pcre2, or other string processing methods).
  // In this example, we use std::regex_replace to perform the actual redaction.
  static std::vector<std::regex> regexes = [] {
    std::vector<std::regex> v;
    for (const auto& pattern : patterns) {
      v.emplace_back(pattern);
    }
    return v;
  }();

  for (const auto& re : regexes) {
    stable_chunk = std::regex_replace(stable_chunk, re, "[REDACTED]");
  }

  std::cout << "[stable-chunk] Redacted stream: " << stable_chunk << '\n';

  // Simulate consumption of the stable chunk after processing
  stable_chunk.clear();
}

void processChunk(std::string&& chunk) {
  const static Sregex::Automata::Dfa dfa(patterns);
  if (!dfa.ok()) {
    std::cerr << "Failed to build DFA: " << dfa.err() << '\n';
    return;
  }

  static std::string buffer;

  const bool has_buffered_data = !buffer.empty();
  if (has_buffered_data) {
    buffer.append(chunk);
  }

  const bool is_partial = dfa.isPartialMatch(has_buffered_data ? buffer : chunk);
  if (!has_buffered_data && is_partial) {
    buffer.append(chunk);
  }

  // If only a partial match is seen, keep buffering until next chunk arrives
  if (is_partial) {
    std::cout << "[partial-hit] keep buffering: \"" << buffer << "\"\n";
    return;
  }

  processStableChunk(has_buffered_data ? std::move(buffer) : std::move(chunk));
}

} // namespace

int main() {
  // Simulate a stream split into chunks where sensitive content may cross chunk boundaries
  std::vector<std::string> chunks = {
      "event=start token_", "AB12", "3X user=trump", "user_id=1", "234 sta", "tus=ok",
  };

  // Simulate processing each chunk as it arrives in the stream
  for (auto& chunk : chunks) {
    std::cout << "[incoming-chunk] " << chunk << std::endl;
    processChunk(std::move(chunk));
    std::cout << "----" << std::endl;
  }

  return 0;
}
