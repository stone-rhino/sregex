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
#include "visitor.h"

#include <cassert>
#include <codecvt>
#include <format>
#include <unordered_map>

#include "hir.h"

#define RETURN_ERROR(msg)                                                                          \
  do {                                                                                             \
    should_visit_next_child_ = false;                                                              \
    return std::format("[{}:{}] {}", ctx->getStart()->getLine(),                                   \
                       ctx->getStart()->getCharPositionInLine(), msg);                             \
  } while (0)

namespace Sregex::Parser {
Visitor::Visitor(Hir& hir) : hir_(hir) {}

std::any Visitor::visitPcre(Antlr4Gen::PCREParser::PcreContext* ctx) {
  // The root HIR node is initialized as an alternation.
  hir_.kind = AlternationKind{};

  // Visit children
  hir_kind_stack_.push(&hir_.kind);
  std::any result = visitChildren(ctx);
  if (result.type() == typeid(std::string)) {
    return result;
  }
  hir_kind_stack_.pop();
  assert(hir_kind_stack_.empty());

  // Flatten the HIR tree
  flatten(&hir_.kind);

  return {};
}

std::any Visitor::visitAlternation(Antlr4Gen::PCREParser::AlternationContext* ctx) {
  // Visit children within a new alternation context
  std::unique_ptr<HirKind> alt_kind = std::make_unique<HirKind>(AlternationKind{});
  hir_kind_stack_.push(alt_kind.get());
  std::any result = visitChildren(ctx);
  hir_kind_stack_.pop();

  // Add the newly created HIR node to the parent
  assert(!hir_kind_stack_.empty());
  flatten(alt_kind.get());
  addHirNode(hir_kind_stack_.top(), std::move(alt_kind));

  return result;
}

std::any Visitor::visitExpr(Antlr4Gen::PCREParser::ExprContext* ctx) {
  // Visit children within a new concatenation context
  std::unique_ptr<HirKind> concat_kind = std::make_unique<HirKind>(ConcatKind{});
  hir_kind_stack_.push(concat_kind.get());
  std::any result = visitChildren(ctx);
  hir_kind_stack_.pop();

  // Add the newly created HIR node to the parent
  assert(!hir_kind_stack_.empty());
  flatten(concat_kind.get());
  addHirNode(hir_kind_stack_.top(), std::move(concat_kind));

  return result;
}

std::any Visitor::visitOption_setting(Antlr4Gen::PCREParser::Option_settingContext* ctx) {
  RETURN_ERROR("unsupported option setting");
}

std::any
Visitor::visitBacktracking_control(Antlr4Gen::PCREParser::Backtracking_controlContext* ctx) {
  RETURN_ERROR("unsupported backtracking control");
}

std::any Visitor::visitCallout(Antlr4Gen::PCREParser::CalloutContext* ctx) {
  RETURN_ERROR("unsupported callout");
}

std::any Visitor::visitCapture(Antlr4Gen::PCREParser::CaptureContext* ctx) {
  RETURN_ERROR("unsupported capture");
}

std::any Visitor::visitAtomic_group(Antlr4Gen::PCREParser::Atomic_groupContext* ctx) {
  RETURN_ERROR("unsupported atomic_group");
}

std::any Visitor::visitLookaround(Antlr4Gen::PCREParser::LookaroundContext* ctx) {
  RETURN_ERROR("unsupported lookaround");
}

std::any Visitor::visitBackreference(Antlr4Gen::PCREParser::BackreferenceContext* ctx) {
  RETURN_ERROR("unsupported backreference");
}

std::any
Visitor::visitSubroutine_reference(Antlr4Gen::PCREParser::Subroutine_referenceContext* ctx) {
  RETURN_ERROR("unsupported subroutine_reference");
}

std::any Visitor::visitConditional_pattern(Antlr4Gen::PCREParser::Conditional_patternContext* ctx) {
  RETURN_ERROR("unsupported conditional_pattern");
}

std::any Visitor::visitComment(Antlr4Gen::PCREParser::CommentContext* ctx) {
  // Skip comments
  return std::any();
}

std::any Visitor::visitQuantifier(Antlr4Gen::PCREParser::QuantifierContext* ctx) {
  std::unique_ptr<HirKind> rep_kind = std::make_unique<HirKind>(RepetitionKind{});
  auto& rep = rep_kind->as<RepetitionKind>();

  auto text = ctx->getText();
  switch (text[0]) {
  case '?':
    rep.min_ = 0;
    rep.max_ = 1;
    break;
  case '*':
    rep.min_ = 0;
    break;
  case '+':
    rep.min_ = 1;
    break;
  case '{':
    std::string min_str = ctx->from->getText();
    std::from_chars(min_str.data(), min_str.data() + min_str.size(), rep.min_);

    auto comma_pos = text.find(',');
    if (comma_pos != std::string::npos) {
      std::string max_str = ctx->to ? ctx->to->getText() : "";
      if (!max_str.empty()) {
        uint32_t max = 0;
        std::from_chars(max_str.data(), max_str.data() + max_str.size(), max);
        rep.max_ = max;
      }
    } else {
      rep.max_ = rep.min_;
    }
    break;
  }

  rep.greedy_ = !ctx->lazy;
  rep.possessive_ = ctx->possessive;

  // Attach the repetition node to the last element of the concatenation kind
  std::visit(
      [&](auto&& parent) {
        using T = std::decay_t<decltype(parent)>;
        if constexpr (std::is_same_v<T, ConcatKind>) {
          if (!parent.empty()) {
            rep.sub_ = std::move(parent.back());
            parent.back() = std::move(rep_kind);
          } else {
            assert(false);
          }
        } else {
          assert(false);
        }
      },
      *hir_kind_stack_.top());

  return std::any();
}

std::any Visitor::visitCharacter(Antlr4Gen::PCREParser::CharacterContext* ctx) {
  // Decode the escape character
  auto decode_escape_result = decodeEscapeCharacter(ctx->getText());
  if (!decode_escape_result) {
    return std::unexpected(decode_escape_result.error());
  }

  appendLiteral(hir_kind_stack_.top(), decode_escape_result.value());

  return {};
}

std::any Visitor::visitCharacter_type(Antlr4Gen::PCREParser::Character_typeContext* ctx) {
  auto char_type_result = decodeEscapeCharacterType(ctx->getText());
  if (!char_type_result) {
    RETURN_ERROR(char_type_result.error());
  }

  addHirNode(hir_kind_stack_.top(), std::make_unique<HirKind>(char_type_result.value()));

  return {};
}

std::any Visitor::visitCharacter_class(Antlr4Gen::PCREParser::Character_classContext* ctx) {
  // Visit children within a new unicode class context
  std::unique_ptr<HirKind> unicode_class_kind = std::make_unique<HirKind>(UnicodeClassKind{});
  hir_kind_stack_.push(unicode_class_kind.get());
  std::any result = visitChildren(ctx);
  hir_kind_stack_.pop();

  // Merge overlapping unicode class ranges
  mergeUnicodeClassRanges(unicode_class_kind->as<UnicodeClassKind>());

  // Handle negation of the character class
  if (ctx->negate) {
    UnicodeClassKind negated_class = makeNegated(unicode_class_kind->as<UnicodeClassKind>());
    *unicode_class_kind = std::move(negated_class);
  }

  // Add the newly created HIR node to the parent
  assert(!hir_kind_stack_.empty());
  addHirNode(hir_kind_stack_.top(), std::move(unicode_class_kind));

  return {};
}

std::any
Visitor::visitCharacter_class_atom(Antlr4Gen::PCREParser::Character_class_atomContext* ctx) {
  // Handle character class ranges
  // e.g., [a-z]
  if (ctx->character_class_range()) {
    assert(ctx->character_class_range()->character_class_range_atom().size() == 2);
    auto* start_atom = ctx->character_class_range()->character_class_range_atom()[0];
    auto* end_atom = ctx->character_class_range()->character_class_range_atom()[1];
    auto start_text = decodeEscapeCharacter(start_atom->getText());
    if (!start_text) {
      RETURN_ERROR(start_text.error());
    }
    auto end_text = decodeEscapeCharacter(end_atom->getText());
    if (!end_text) {
      RETURN_ERROR(end_text.error());
    }

    // Convert UTF-8 to UTF-32
    std::u32string start_u32 = utf8ToUtf32(start_text.value());
    std::u32string end_u32 = utf8ToUtf32(end_text.value());
    assert(start_u32.size() == 1);
    assert(end_u32.size() == 1);
    if (start_u32.size() != 1 || end_u32.size() != 1) {
      RETURN_ERROR("Invalid character range");
    }

    // Get the Unicode code points of the start and end characters
    auto start_code_point = static_cast<uint32_t>(start_u32[0]);
    auto end_code_point = static_cast<uint32_t>(end_u32[0]);
    if (start_code_point > end_code_point) {
      RETURN_ERROR("Invalid character range: start code point is greater than end code point");
    }

    std::visit(
        [&](auto&& unicode_class) {
          using T = std::decay_t<decltype(unicode_class)>;
          if constexpr (std::is_same_v<T, UnicodeClassKind>) {
            unicode_class.ranges_.emplace_back(start_code_point, end_code_point);
          } else {
            assert(false);
          }
        },
        *hir_kind_stack_.top());
  }

  // Handle POSIX character classes
  // e.g., [:alnum:]
  else if (ctx->posix_character_class()) {
    bool inner_negated = ctx->posix_character_class()->negate != nullptr;
    auto posix_class_name = ctx->posix_character_class()->letters()->getText();
    auto class_kinds_result = getPosixCharacterClass(posix_class_name, inner_negated);
    if (!class_kinds_result) {
      RETURN_ERROR(class_kinds_result.error());
    }
    auto class_kinds = class_kinds_result.value();
    std::visit(
        [&](auto&& unicode_class) {
          using T = std::decay_t<decltype(unicode_class)>;
          if constexpr (std::is_same_v<T, UnicodeClassKind>) {
            for (const auto& range : class_kinds.ranges_) {
              unicode_class.ranges_.emplace_back(range);
            }
          } else {
            assert(false);
          }
        },
        *hir_kind_stack_.top());
  }

  // Handle escaped characters
  // e.g., \a \f
  else if (ctx->character()) {
    auto result = decodeEscapeCharacter(ctx->character()->getText());
    if (!result) {
      RETURN_ERROR(result.error());
    }

    // Convert UTF-8 to UTF-32
    std::u32string char_u32 = utf8ToUtf32(result.value());

    // Get the Unicode code points of the character
    auto code_point = static_cast<uint32_t>(char_u32[0]);

    std::visit(
        [&](auto&& unicode_class) {
          using T = std::decay_t<decltype(unicode_class)>;
          if constexpr (std::is_same_v<T, UnicodeClassKind>) {
            unicode_class.ranges_.emplace_back(code_point, code_point);
          } else {
            assert(false);
          }
        },
        *hir_kind_stack_.top());
  }

  // Handle character types
  // e.g., \d \w \s
  else if (ctx->character_type()) {
    auto char_type_name = ctx->character_type()->getText();
    auto char_type_result = decodeEscapeCharacterType(char_type_name);
    if (!char_type_result) {
      RETURN_ERROR(char_type_result.error());
    }
    auto char_type = char_type_result.value();
    std::visit(
        [&](auto&& unicode_class) {
          using T = std::decay_t<decltype(unicode_class)>;
          if constexpr (std::is_same_v<T, UnicodeClassKind>) {
            for (const auto& range : char_type.ranges_) {
              unicode_class.ranges_.emplace_back(range);
            }
          } else {
            assert(false);
          }
        },
        *hir_kind_stack_.top());
  }

  // Handle any other characters
  else {
    auto text = ctx->getText();

    UnicodeClassKind::Range range;
    if (text == "\\.") {
      range = UnicodeClassKind::Range('.', '.');
    } else {
      // Convert UTF-8 to UTF-32
      std::u32string char_u32 = utf8ToUtf32(text);
      range = UnicodeClassKind::Range(char_u32[0], char_u32[0]);
    }

    std::visit(
        [&](auto&& unicode_class) {
          using T = std::decay_t<decltype(unicode_class)>;
          if constexpr (std::is_same_v<T, UnicodeClassKind>) {
            unicode_class.ranges_.emplace_back(range);
          } else {
            assert(false);
          }
        },
        *hir_kind_stack_.top());
  }
  return {};
}

std::any
Visitor::visitPosix_character_class(Antlr4Gen::PCREParser::Posix_character_classContext* ctx) {
  RETURN_ERROR("unsupported posix_character_class");
}

std::any Visitor::visitLetter(Antlr4Gen::PCREParser::LetterContext* ctx) {
  appendLiteral(hir_kind_stack_.top(), ctx->getText());
  return {};
}

std::any Visitor::visitDigit(Antlr4Gen::PCREParser::DigitContext* ctx) {
  appendLiteral(hir_kind_stack_.top(), ctx->getText());
  return {};
}

std::any Visitor::visitAnchor(Antlr4Gen::PCREParser::AnchorContext* ctx) {
  auto parent = hir_kind_stack_.top();
  auto text = ctx->getText();
  switch (text[0]) {
  case '^':
    addHirNode(parent, std::make_unique<HirKind>(AnchorKind::Start));
    break;
  case '$':
    addHirNode(parent, std::make_unique<HirKind>(AnchorKind::End));
    break;
  case '\\':
    if (text.size() >= 2) {
      switch (text[1]) {
      case 'b':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::WordBoundary));
        break;
      case 'B':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::NonWordBoundary));
        break;
      case 'A':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::Start));
        break;
      case 'z':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::End));
        break;
      case 'Z':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::EndLine));
        break;
      case 'G':
        addHirNode(parent, std::make_unique<HirKind>(AnchorKind::LastMatchPosition));
        break;
      default:
        assert(false);
      }
    }
    break;
  }

  return {};
}

std::any Visitor::visitMatch_point_reset(Antlr4Gen::PCREParser::Match_point_resetContext* ctx) {
  RETURN_ERROR("unsupported match_point_reset");
}

std::any Visitor::visitQuoting(Antlr4Gen::PCREParser::QuotingContext* ctx) {
  RETURN_ERROR("unsupported quoting");
}

std::any Visitor::visitOther(Antlr4Gen::PCREParser::OtherContext* ctx) {
  auto result = decodeEscapeCharacter(ctx->getText());
  if (!result) {
    return std::unexpected(result.error());
  }

  appendLiteral(hir_kind_stack_.top(), result.value());

  return {};
}

std::expected<std::string, std::string> Visitor::decodeEscapeCharacter(const std::string& text) {
  if (text.empty() || text[0] != '\\') {
    return text;
  }

  switch (text[1]) {
  case 'a':
    return "\a";
  case 'c': {
    if (text.size() >= 3) {
      char control_char = text[2];
      if ((control_char >= 'a' && control_char <= 'z') ||
          (control_char >= 'A' && control_char <= 'Z')) {
        return std::string(1, (control_char & 0x1F));
      }
    }
    return text;
  }
  case 'e':
    return "\e";
  case 'f':
    return "\f";
  case 'n':
    return "\n";
  case 'r':
    return "\r";
  case 't':
    return "\t";
  case '0':
    [[fallthrough]];
  case '1':
    [[fallthrough]];
  case '2':
    [[fallthrough]];
  case '3':
    [[fallthrough]];
  case '4':
    [[fallthrough]];
  case '5':
    [[fallthrough]];
  case '6':
    [[fallthrough]];
  case '7':
    [[fallthrough]];
  case '8':
    [[fallthrough]];
  case '9':
    [[fallthrough]];
  case 'o':
    [[fallthrough]];
  case 'x':
    [[fallthrough]];
  case 'u':
    // FIXME(zhouyu 2026-09-18): Implement handling for octal, hexadecimal, and Unicode escape
    return std::unexpected("unsupported escape sequence");
  default:
    return text;
  }
}

std::expected<UnicodeClassKind, std::string>
Visitor::decodeEscapeCharacterType(const std::string& text) {
  // Table of character types
  const static std::unordered_map<std::string, UnicodeClassKind> char_type_map = {
      {".", {{{0x00, 0x10FFFF}}}},
      {"\\d", {{{0x30, 0x39}}}},
      {"\\D", {{{0x00, 0x2F}, {0x3A, 0x10FFFF}}}},
      {"\\s", {{{0x09, 0x0A}, {0x0C, 0x0D}, {0x20, 0x20}}}},
      {"\\S", {{{0x00, 0x08}, {0x0B, 0x0B}, {0x0E, 0x1F}, {0x21, 0x10FFFF}}}},
      {"\\w", {{{0x30, 0x39}, {0x41, 0x5A}, {0x5F, 0x5F}, {0x61, 0x7A}}}},
      {"\\W", {{{0x00, 0x2F}, {0x3A, 0x40}, {0x5B, 0x5E}, {0x60, 0x60}, {0x7B, 0x10FFFF}}}},
  };

  auto iter = char_type_map.find(text);
  if (iter == char_type_map.end()) {
    return std::unexpected("unknown character type: " + text);
  }
  return iter->second;
}

void Visitor::mergeUnicodeClassRanges(UnicodeClassKind& class_kind) {
  auto& ranges = class_kind.ranges_;
  if (ranges.empty()) {
    return;
  }

  std::sort(ranges.begin(), ranges.end(), [](const auto& a, const auto& b) {
    if (a.start_ < b.start_) {
      return true;
    } else if (a.start_ == b.start_) {
      return a.end_ < b.end_;
    } else {
      return false;
    }
  });

  std::vector<UnicodeClassKind::Range> merged;
  merged.push_back(ranges[0]);
  for (size_t i = 1; i < ranges.size(); ++i) {
    auto& last = merged.back();
    if (ranges[i].start_ <= last.end_ + 1) {
      last.end_ = std::max(last.end_, ranges[i].end_);
    } else {
      merged.push_back(ranges[i]);
    }
  }
  ranges = std::move(merged);
}

std::expected<UnicodeClassKind, std::string>
Visitor::getPosixCharacterClass(const std::string& name, bool negate) {
  // Table of POSIX character classes
  const static std::unordered_map<std::string, UnicodeClassKind> posix_class_map = {
      {"alnum", {{{0x30, 0x39}, {0x41, 0x5A}, {0x61, 0x7A}}}},
      {"alpha", {{{0x41, 0x5A}, {0x61, 0x7A}}}},
      {"ascii", {{{0x00, 0x7F}}}},
      {"blank", {{{0x09, 0x09}, {0x20, 0x20}}}},
      {"cntrl", {{{0x00, 0x1F}, {0x7F, 0x7F}}}},
      {"digit", {{{0x30, 0x39}}}},
      {"graph", {{{0x21, 0x7E}}}},
      {"lower", {{{0x61, 0x7A}}}},
      {"print", {{{0x20, 0x7E}}}},
      {"punct", {{{0x21, 0x2F}, {0x3A, 0x40}, {0x5B, 0x60}, {0x7B, 0x7E}}}},
      {"space", {{{0x09, 0x0D}, {0x20, 0x20}}}},
      {"upper", {{{0x41, 0x5A}}}},
      {"word", {{{0x30, 0x39}, {0x41, 0x5A}, {0x5F, 0x5F}, {0x61, 0x7A}}}},
      {"xdigit", {{{0x30, 0x39}, {0x41, 0x46}, {0x61, 0x66}}}}};

  // Precompute the negated POSIX character classes
  static const std::unordered_map<std::string, UnicodeClassKind> posix_class_negated_map = [&] {
    std::unordered_map<std::string, UnicodeClassKind> t;
    t.reserve(posix_class_map.size());
    for (const auto& [k, v] : posix_class_map) {
      t.emplace(k, makeNegated(v));
    }
    return t;
  }();

  const auto& table = negate ? posix_class_negated_map : posix_class_map;

  auto iter = table.find(name);
  if (iter == table.end()) {
    return std::unexpected("unknown POSIX character class: " + name);
  }

  return iter->second;
}

UnicodeClassKind Visitor::makeNegated(const UnicodeClassKind& class_kind) {
  constexpr uint32_t MAX = 0x10FFFF;
  UnicodeClassKind out;
  uint32_t next = 0;
  for (const auto& [start, end] : class_kind.ranges_) {
    if (next < start) {
      out.ranges_.emplace_back(next, start - 1);
    }
    next = end + 1;
  }
  if (next <= MAX) {
    out.ranges_.emplace_back(next, MAX);
  }
  return out;
}

std::u32string Visitor::utf8ToUtf32(const std::string& text) {
  static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  return conv.from_bytes(text);
}

void Visitor::flatten(HirKind* kind) {
  std::visit(
      [&](auto&& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, AlternationKind> || std::is_same_v<T, ConcatKind>) {
          // If there is only one element in the AlternationKind or ConcatKind, use it directly
          if (node.size() == 1) {
            std::unique_ptr<HirKind> front = std::move(node.front());
            *kind = std::move(*front);

            // Recursively flatten the subexpression
            flatten(kind);
          }
        }
      },
      *kind);
}

void Visitor::appendLiteral(HirKind* parent, const std::string& text) {
  assert(parent);
  if (parent) {
    std::visit(
        [&](auto&& kind) {
          using T = std::decay_t<decltype(kind)>;
          if constexpr (std::is_same_v<T, AlternationKind> || std::is_same_v<T, ConcatKind>) {
            if (!kind.empty() && kind.back()->template is<LiteralKind>()) {
              // Merge consecutive letter literals into a single LiteralKind
              auto& literal = kind.back()->template as<LiteralKind>();
              literal += text;
            } else {
              auto node = std::make_unique<HirKind>(LiteralKind(text));
              kind.push_back(std::move(node));
            }
          } else {
            assert(false);
          }
        },
        *parent);
  }
}

void Visitor::addHirNode(HirKind* parent, std::unique_ptr<HirKind>&& child) {
  assert(parent);
  if (parent) {
    std::visit(
        [&](auto&& kind) {
          using T = std::decay_t<decltype(kind)>;
          if constexpr (std::is_same_v<T, AlternationKind> || std::is_same_v<T, ConcatKind>) {
            kind.push_back(std::move(child));
          } else {
            assert(false);
          }
        },
        *parent);
  }
}

} // namespace Sregex::Parser