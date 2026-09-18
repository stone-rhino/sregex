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
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace Sregex::Parser {
struct Hir;

// The kind representing a literal string
struct LiteralKind : public std::string {};

// The kind representing a Unicode character class
struct UnicodeClassKind {
  // The starting code point of the Unicode character class
  uint32_t start_;
  // The ending code point of the Unicode character class
  uint32_t end_;
};

// The kind representing a byte character class
struct ByteClassKind {
  // The starting byte of the byte character class
  uint8_t start_;
  // The ending byte of the byte character class
  uint8_t end_;
};

// The kind representing a lookahead or lookbehind assertion
struct LookKind {};

// The kind representing a repetition
struct RepetitionKind {
  // The minimum range of repetition
  uint32_t min_;
  // The maximum range of repetition, if any.
  std::optional<uint32_t> max_;
  // Whether the repetition is greedy
  bool greedy_;
  // The subexpression being repeated
  std::unique_ptr<Hir> sub_;
};

// The kind representing a capture group
struct CaptureKind {
  // The index of the capture group
  uint32_t index_;
  // The name of the capture group, if any.
  std::string name_;
  // The subexpression captured by this group, which may be empty.
  std::unique_ptr<Hir> sub_;
};

// The kind representing a concatenation of subexpressions
struct ConcatKind : public std::vector<std::unique_ptr<Hir>> {};

// The kind representing an alternation of subexpressions
struct AlternationKind : public std::vector<std::unique_ptr<Hir>> {};

// The variant type representing all possible kinds of HIR nodes
using HirKind = std::variant<LiteralKind, UnicodeClassKind, ByteClassKind, LookKind, RepetitionKind,
                             CaptureKind, ConcatKind, AlternationKind>;
} // namespace Sregex::Parser