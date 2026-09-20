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

#include "hir.h"

#define RETURN_ERROR(msg)                                                                          \
  do {                                                                                             \
    should_visit_next_child_ = false;                                                              \
    return std::format("[{}:{}] {}", ctx->getStart()->getLine(),                                   \
                       ctx->getStart()->getCharPositionInLine(), msg);                             \
  } while (0)

#define RETURN_IF_ERROR(expr)                                                                      \
  do {                                                                                             \
    auto result = (expr);                                                                          \
    if (!result) {                                                                                 \
      RETURN_ERROR(result.error());                                                                \
    }                                                                                              \
  } while (0)

namespace Sregex::Parser {

Visitor::Visitor(Hir& hir) : hir_(hir) {}

std::any Visitor::visitExpr(Antlr4Gen::PCREParser::ExprContext* ctx) {
  hir_.kind = ConcatKind{};
  auto& concat = hir_.kind.as<ConcatKind>();

  for (auto* element : ctx->element()) {
    auto* atom = element->atom();
    if (atom->option_setting() != nullptr) {
      RETURN_ERROR("unsupported option setting");
    } else if (atom->backtracking_control() != nullptr) {
      RETURN_ERROR("unsupported backtracking control");
    } else if (atom->callout() != nullptr) {
      RETURN_ERROR("unsupported callout");
    } else if (atom->capture() != nullptr) {
      RETURN_ERROR("unsupported capture");
    } else if (atom->atomic_group() != nullptr) {
      RETURN_ERROR("unsupported atomic_group");
    } else if (atom->lookaround() != nullptr) {
      RETURN_ERROR("unsupported lookaround");
    } else if (atom->backreference() != nullptr) {
      RETURN_ERROR("unsupported backreference");
    } else if (atom->subroutine_reference() != nullptr) {
      RETURN_ERROR("unsupported subroutine_reference");
    } else if (atom->conditional_pattern() != nullptr) {
      RETURN_ERROR("unsupported conditional_pattern");
    }

    // Skip comments
    else if (atom->comment() != nullptr) {
      continue;
    }

    // \a or \c or other escape sequences
    else if (atom->character() != nullptr) {
      RETURN_IF_ERROR(buildLiteral(atom->character()->getText(), true));
    } else if (atom->character_type() != nullptr) {
      RETURN_ERROR("unsupported character_type");
    }

    // [abc] or [^abc] or [a-z] etc.
    else if (atom->character_class() != nullptr) {
      RETURN_IF_ERROR(buildUnicodeClass(atom->character_class()));
    } else if (atom->posix_character_class() != nullptr) {
      RETURN_ERROR("unsupported posix_character_class");
    }

    // Single letters like 'a', 'b', 'c', etc.
    else if (atom->letter() != nullptr) {
      RETURN_IF_ERROR(buildLiteral(atom->letter()->getText()));
    }

    // Single digits like '0', '1', '2', etc.
    else if (atom->digit() != nullptr) {
      RETURN_IF_ERROR(buildLiteral(atom->digit()->getText()));
    } else if (atom->anchor() != nullptr) {
      RETURN_ERROR("unsupported anchor");
    } else if (atom->match_point_reset() != nullptr) {
      RETURN_ERROR("unsupported match_point_reset");
    } else if (atom->quoting() != nullptr) {
      RETURN_ERROR("unsupported quoting");
    } else if (atom->other() != nullptr) {
      RETURN_IF_ERROR(buildLiteral(atom->other()->getText(), true));
    }
  }

  // If there is only one element in the concatenation, use it directly
  if (concat.size() == 1) {
    auto front = std::move(concat.front()->kind);
    hir_.kind = std::move(front);
  }

  return {};
}

std::expected<void, std::string> Visitor::buildLiteral(const std::string& text, bool escape) {
  std::expected<std::string, std::string> result;
  const std::string* decoded_text_ptr = &text;
  if (escape) {
    result = decodeEscapeCharacter(text);
    if (!result) {
      return std::unexpected(result.error());
    }
    decoded_text_ptr = &result.value();
  }

  auto& concat = hir_.kind.as<ConcatKind>();
  if (!concat.empty() && concat.back()->kind.is<LiteralKind>()) {
    // Merge consecutive letter literals into a single LiteralKind
    auto& literal = std::get<LiteralKind>(concat.back()->kind);
    literal += *decoded_text_ptr;
  } else {
    auto node = std::make_unique<Hir>();
    node->kind = LiteralKind(*decoded_text_ptr);
    concat.emplace_back(std::move(node));
  }
  return {};
}

std::expected<void, std::string>
Visitor::buildUnicodeClass(Antlr4Gen::PCREParser::Character_classContext* ctx) {
  // Converter for UTF-8 to UTF-32
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

  bool negated = ctx->negate != nullptr;
  std::vector<UnicodeClassKind> class_kinds;
  for (auto* atom : ctx->character_class_atom()) {
    if (atom->character_class_range()) {
      assert(atom->character_class_range()->character_class_range_atom().size() == 2);
      auto* start_atom = atom->character_class_range()->character_class_range_atom()[0];
      auto* end_atom = atom->character_class_range()->character_class_range_atom()[1];
      auto start_text = decodeEscapeCharacter(start_atom->getText());
      if (!start_text) {
        return std::unexpected(start_text.error());
      }
      auto end_text = decodeEscapeCharacter(end_atom->getText());
      if (!end_text) {
        return std::unexpected(end_text.error());
      }

      // Convert UTF-8 to UTF-32
      std::u32string start_u32 = conv.from_bytes(start_text.value());
      std::u32string end_u32 = conv.from_bytes(end_text.value());
      assert(start_u32.size() == 1);
      assert(end_u32.size() == 1);
      if (start_u32.size() != 1 || end_u32.size() != 1) {
        return std::unexpected("Invalid character range");
      }

      // Get the Unicode code points of the start and end characters
      auto start_code_point = static_cast<uint32_t>(start_u32[0]);
      auto end_code_point = static_cast<uint32_t>(end_u32[0]);
      if (start_code_point > end_code_point) {
        return std::unexpected(
            "Invalid character range: start code point is greater than end code point");
      }

      class_kinds.push_back(UnicodeClassKind{start_code_point, end_code_point});
    } else if (atom->posix_character_class()) {
      bool inner_negated = atom->posix_character_class()->negate != nullptr;
      auto text = atom->posix_character_class()->letters()->getText();
    } else if (atom->character()) {
      auto text = decodeEscapeCharacter(atom->character()->getText());
    } else if (atom->character_type()) {
      auto text = decodeEscapeCharacterType(atom->character_type()->getText());
    } else {
      auto text = atom->getText();
    }
  }

  auto& concat = hir_.kind.as<ConcatKind>();
  for (auto& class_kind : class_kinds) {
    auto node = std::make_unique<Hir>();
    node->kind = class_kind;
    concat.push_back(std::move(node));
  }
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

std::expected<std::string, std::string>
Visitor::decodeEscapeCharacterType(const std::string& text) {
  // FIXME(zhouyu 2026-09-20): Implement handling for escape character types
  return std::unexpected("unsupported escape character type");
}

} // namespace Sregex::Parser