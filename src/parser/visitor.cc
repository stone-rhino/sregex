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

#include <format>

#include "hir.h"

#define RETURN_ERROR(msg)                                                                          \
  should_visit_next_child_ = false;                                                                \
  return std::format("[{}:{}] {}", ctx->getStart()->getLine(),                                     \
                     ctx->getStart()->getCharPositionInLine(), msg);

namespace Sregex::Parser {

Visitor::Visitor(Hir& hir) : hir_(hir) {}

std::any Visitor::visitExpr(Antlr4Gen::PCREParser::ExprContext* ctx) {
  hir_.kind = ConcatKind{};
  auto& concat = hir_.kind.as<ConcatKind>();

  for (auto* element : ctx->element()) {
    if (element == nullptr || element->atom() == nullptr) {
      RETURN_ERROR("unsupported expr element");
    }

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
    } else if (atom->comment() != nullptr) {
      // Skip comments
      continue;
    } else if (atom->character() != nullptr) {
      auto result = decodeEscape(atom->character()->getText());
      if (!result) {
        RETURN_ERROR(result.error());
      }
      appendToLiteral(result.value());
    } else if (atom->character_type() != nullptr) {
      RETURN_ERROR("unsupported character_type");
    } else if (atom->character_class() != nullptr) {
      RETURN_ERROR("unsupported character_class");
    } else if (atom->posix_character_class() != nullptr) {
      RETURN_ERROR("unsupported posix_character_class");
    } else if (atom->letter() != nullptr) {
      appendToLiteral(atom->letter()->getText());
    } else if (atom->digit() != nullptr) {
      appendToLiteral(atom->digit()->getText());
    } else if (atom->anchor() != nullptr) {
      RETURN_ERROR("unsupported anchor");
    } else if (atom->match_point_reset() != nullptr) {
      RETURN_ERROR("unsupported match_point_reset");
    } else if (atom->quoting() != nullptr) {
      RETURN_ERROR("unsupported quoting");
    } else if (atom->other() != nullptr) {
      RETURN_ERROR("unsupported other");
    }
  }

  // If there is only one element in the concatenation, use it directly
  if (concat.size() == 1) {
    auto front = std::move(concat.front()->kind);
    hir_.kind = std::move(front);
  }

  return {};
}

void Visitor::appendToLiteral(const std::string& text) {
  auto& concat = hir_.kind.as<ConcatKind>();
  if (!concat.empty() && concat.back()->kind.is<LiteralKind>()) {
    // Merge consecutive letter literals into a single LiteralKind
    auto& literal = std::get<LiteralKind>(concat.back()->kind);
    literal += text;
  } else {
    auto node = std::make_unique<Hir>();
    node->kind = LiteralKind(text);
    concat.emplace_back(std::move(node));
  }
}

std::expected<std::string, std::string> Visitor::decodeEscape(const std::string& text) {
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

} // namespace Sregex::Parser