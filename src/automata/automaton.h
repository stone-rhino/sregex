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
#pragma once

#include <cstring>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace Sregex::Automata {
struct MatchInfo;

template <class Derived> class Automaton {
public:
  /**
   * Checks if the input string matches the automaton's patterns
   *
   * @param input The input string to check
   * @param length The length of the input string
   * @return True if the input matches the pattern, false otherwise
   *
   * @note This method is overloaded to accept std::string, std::string_view, and C-style strings
   */
  bool isMatch(const std::string& input) const { return isMatch(input.data(), input.size()); }
  bool isMatch(std::string_view input) const { return isMatch(input.data(), input.size()); }
  bool isMatch(const char* input) const { return isMatch(input, std::strlen(input)); }
  bool isMatch(const char* input, size_t length) const {
    return derived().isMatchImpl(input, length);
  }

  /**
   * Checks if the input string partially matches the automaton's patterns
   *
   * @param input The input string to check. When performing partial matching, only the last few
   * bytes of the input are considered based on the longestPath.
   * @param length The length of the input string
   * @return True if the input partially matches the pattern, false otherwise
   *
   * @note This method is overloaded to accept std::string, std::string_view, and C-style strings
   */
  bool isPartialMatch(const std::string& input) const {
    return isPartialMatch(input.data(), input.size());
  }
  bool isPartialMatch(std::string_view input) const {
    return isPartialMatch(input.data(), input.size());
  }
  bool isPartialMatch(const char* input) const { return isPartialMatch(input, std::strlen(input)); }
  bool isPartialMatch(const char* input, size_t length) const {
    return derived().isPartialMatchImpl(input, length);
  }

  /**
   * Returns the length of the longest(acyclic) path in the automaton's state graph.
   */
  size_t longestPath() const { return derived().longestPathImpl(); }

  /**
   * Converts the automaton to DOT format
   *
   * @return A string representing the automaton in DOT format
   */
  std::string toDot() const { return derived().toDotImpl(); }

  /**
   * Converts the automaton to DOT format and saves it as an SVG file
   * @param outputFile The path to the output SVG file
   * @note This method requires Graphviz's dot command to be available in the system's PATH. Use
   * `apt install graphviz` to install it on ubuntu systems.
   */
  void toDotSvg(const std::string& outputFile) const {
    std::string dot = toDot();
    std::string dot_file = outputFile + ".dot";
    {
      std::ofstream ofs(dot_file);
      if (ofs) {
        ofs << dot;
      }
    }
    std::string command = "dot -Tsvg -o " + outputFile + " " + dot_file;
    FILE* pipe = ::popen(command.c_str(), "r");
    if (pipe) {
      ::pclose(pipe);
    }
  }

private:
  const Derived& derived() const { return static_cast<const Derived&>(*this); }
};
} // namespace Sregex::Automata