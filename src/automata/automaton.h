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
   * @param partial If true, allows partial matches
   * @return True if the input matches the pattern, false otherwise
   *
   * @note This method is overloaded to accept std::string, std::string_view, and C-style strings
   */
  bool isMatch(const std::string& input, bool partial = false) const {
    return isMatch(input.data(), input.size(), partial);
  }
  bool isMatch(std::string_view input, bool partial = false) const {
    return isMatch(input.data(), input.size(), partial);
  }
  bool isMatch(const char* input, bool partial = false) const {
    return isMatch(input, std::strlen(input), partial);
  }
  bool isMatch(const char* input, size_t length, bool partial = false) const {
    return derived().isMatchImpl(input, length, partial);
  }

  /**
   * Converts the automaton to DOT format
   *
   * @return A string representing the automaton in DOT format
   */
  std::string toDot() const { return derived().toDotImpl(); }

  /**
   * Converts the automaton to DOT format and saves it as an SVG file
   * @param outputFile The path to the output SVG file
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