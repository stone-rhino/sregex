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

#include <cstdint>
#include <expected>
#include <optional>
#include <span>

#include "nfa.h"
#include "src/parser/hir.h"

namespace Sregex::Automata {
/**
 * A Factory class using Thompson's construction to build NFAs from various inputs
 */
class NfaBuilder {
public:
  static Nfa withPattern(const std::string& pattern);
  static Nfa withPatterns(const std::span<const std::string>& patterns);
  static Nfa withHir(const Parser::Hir& hir);
  static Nfa withChar(char c);
  static Nfa withString(const std::string& str);
  static Nfa withRange(uint32_t start, uint32_t end);
  static Nfa withConcat(const Nfa& first, const Nfa& second);
  static Nfa withAlternation(const std::span<const Nfa>& nfas);
  static Nfa withKleeneStar(const Nfa& base_nfa);
  static Nfa withPlus(const Nfa& base_nfa);
  static Nfa withQuestionMark(const Nfa& base_nfa);
  static Nfa withExactRepetition(const Nfa& base_nfa, size_t count);
  static std::expected<Nfa, std::string> withRangeRepetition(const Nfa& base_nfa, size_t min,
                                                             std::optional<size_t> max);
};
} // namespace Sregex::Automata