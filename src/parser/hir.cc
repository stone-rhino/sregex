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

#include "hir.h"

#include <format>

#include "antlr4_gen/PCREBaseVisitor.h"
#include "antlr4_gen/PCRELexer.h"
#include "antlr4_gen/PCREParser.h"
#include "antlr4_gen/PCREVisitor.h"
#include "visitor.h"

namespace Sregex::Parser {
class ParserErrorListener : public antlr4::BaseErrorListener {
public:
  ParserErrorListener(std::string_view file_path) : file_path_(file_path) {}

public:
  void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string& msg,
                   std::exception_ptr e) override {
    if (file_path_.empty()) {
      error_msg = std::format("PCRE parser error. [{}:{}] {}", line, charPositionInLine, msg);
    } else {
      error_msg = std::format("PCRE parser error. [{}:{}:{}] {}", file_path_, line,
                              charPositionInLine, msg);
    }
  }

public:
  std::string error_msg;

private:
  std::string_view file_path_;
};

class LexerErrorListener : public antlr4::BaseErrorListener {
public:
  void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
                   size_t charPositionInLine, const std::string& msg,
                   std::exception_ptr e) override {
    error_msg = std::format("PCRE lexer error. {}:{} {}", line, charPositionInLine, msg);
  }

public:
  std::string error_msg;
};

void dump_helper(const Hir& hir, std::string& out) {
  std::visit(
      [&](auto&& kind) {
        if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, EmptyKind>) {
          out += "empty";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, LiteralKind>) {
          out += "lit{" + kind + "}";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, UnicodeClassKind>) {
          out += "uc{";
          for (size_t i = 0; i < kind.ranges_.size(); ++i) {
            const auto& range = kind.ranges_[i];
            if (i != 0) {
              out += " ";
            }
            if (range.start_ == range.end_) {
              out += std::format("0x{:02x}", range.start_);
            } else {
              out += std::format("0x{:02x}-0x{:02x}", range.start_, range.end_);
            }
          }
          out += "}";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, AnchorKind>) {
          std::string name;
          switch (kind) {
          case AnchorKind::Start:
            name = "Start";
            break;
          case AnchorKind::End:
            name = "End";
            break;
          case AnchorKind::StartLine:
            name = "StartLine";
            break;
          case AnchorKind::EndLine:
            name = "EndLine";
            break;
          case AnchorKind::WordBoundary:
            name = "WordBoundary";
            break;
          case AnchorKind::NonWordBoundary:
            name = "NonWordBoundary";
            break;
          case AnchorKind::LastMatchPosition:
            name = "LastMatchPosition";
            break;
          default:
            name = "Unknown";
            break;
          }
          out += "anc{" + name + "}";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, RepetitionKind>) {
          std::string temp;
          dump_helper(*kind.sub_, temp);

          std::string rep_count =
              std::format("{},{} ", kind.min_,
                          kind.max_.has_value() ? static_cast<int64_t>(kind.max_.value()) : -1);
          out += "rep{" + rep_count + temp + "}";

        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, CaptureKind>) {
          std::string temp;
          dump_helper(*kind.sub_, temp);
          out += "cap{" + temp + "}";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, ConcatKind>) {
          std::string temp;
          for (const std::unique_ptr<Hir>& sub_hir : kind) {
            if (!temp.empty()) {
              temp += " ";
            }
            dump_helper(*sub_hir, temp);
          }
          out += "cat{" + temp + "}";
        } else if constexpr (std::is_same_v<std::decay_t<decltype(kind)>, AlternationKind>) {
          std::string temp;
          for (const std::unique_ptr<Hir>& sub_hir : kind) {
            if (!temp.empty()) {
              temp += " | ";
            }
            dump_helper(*sub_hir, temp);
          }

          out += "alt{" + temp + "}";
        }
      },
      hir.kind);
}

std::string Hir::dump() const {
  std::string out;
  dump_helper(*this, out);
  return out;
}

std::expected<std::unique_ptr<Hir>, std::string> Hir::parse(const char* pattern, size_t length) {
  // Initialize
  antlr4::ANTLRInputStream input(pattern, length);
  Antlr4Gen::PCRELexer lexer(&input);
  antlr4::CommonTokenStream tokens(&lexer);
  Antlr4Gen::PCREParser parser(&tokens);

  // Sets error listener
  ParserErrorListener parser_error_listener("");
  LexerErrorListener lexer_error_listener;
  // parser.setBuildParseTree(true);
  parser.removeErrorListeners();
  parser.addErrorListener(&parser_error_listener);
  lexer.removeErrorListeners();
  lexer.addErrorListener(&lexer_error_listener);

  // Parse
  auto ast = parser.pcre();

  // Visit
  std::unique_ptr<Hir> hir = std::make_unique<Hir>();
  if (length != 0) {
    Visitor visitor(*hir);
    std::any result = visitor.visit(ast);
    if (result.type() == typeid(std::string)) {
      return std::unexpected(std::any_cast<std::string>(result));
    }
  } else {
    hir->kind = EmptyKind{};
  }

  return hir;
}

} // namespace Sregex::Parser
