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
struct HirKind;

// The kind representing an empty expression, which matches everything, including the empty string.
struct EmptyKind {};

// The kind representing a literal string
struct LiteralKind : public std::string {};

// The kind representing a unicode character class
struct UnicodeClassKind {
  struct Range {
    // The starting code point of the unicode character class
    uint32_t start_;
    // The ending code point of the unicode character class
    uint32_t end_;
  };
  std::vector<Range> ranges_;
};

// The kind representing an anchor
enum class AnchorKind {
  // ^ or \A: Match the start of the string
  Start,
  // $ or \z: Match the end of the string
  End,
  // ^(multiline mode): Match the start of a line or start of the string
  StartLine,
  // $(multiline mode) or \Z: Match the end of a line or end of the string
  EndLine,
  // \b: Match an ASCII word boundary
  WordBoundary,
  // \B: Match an ASCII non-word boundary
  NonWordBoundary,
  // \G: Match the last matched position in the string
  LastMatchPosition
};

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
struct ConcatKind : public std::vector<std::unique_ptr<HirKind>> {
private:
  using Base = std::vector<std::unique_ptr<HirKind>>;

public:
  using Base::Base;
};

// The kind representing an alternation of subexpressions
struct AlternationKind : public std::vector<std::unique_ptr<HirKind>> {
private:
  using Base = std::vector<std::unique_ptr<HirKind>>;

public:
  using Base::Base;
};

// The variant type representing all possible kinds of HIR nodes
struct HirKind : public std::variant<EmptyKind, LiteralKind, UnicodeClassKind, AnchorKind,
                                     RepetitionKind, CaptureKind, ConcatKind, AlternationKind> {
private:
  using Base = std::variant<EmptyKind, LiteralKind, UnicodeClassKind, AnchorKind, RepetitionKind,
                            CaptureKind, ConcatKind, AlternationKind>;

public:
  using Base::Base;
  template <typename T> bool is() const { return std::holds_alternative<T>(*this); }
  template <typename T> T& as() { return std::get<T>(*this); }
  template <typename T> const T& as() const { return std::get<T>(*this); }
};
} // namespace Sregex::Parser