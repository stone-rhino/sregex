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

#include <expected>

#include "antlr4_gen/PCREBaseVisitor.h"

namespace Sregex::Parser {
class Hir;
class Visitor final : public Antlr4Gen::PCREBaseVisitor {
public:
  Visitor(Hir& hir);

protected:
  bool shouldVisitNextChild(antlr4::tree::ParseTree*, const std::any&) override {
    return should_visit_next_child_;
  }
  std::any visitExpr(Antlr4Gen::PCREParser::ExprContext* ctx) override;

private:
  [[nodiscard]]
  std::expected<void, std::string> buildLiteral(const std::string& text, bool escape = false);
  [[nodiscard]]
  std::expected<void, std::string>
  buildUnicodeClass(Antlr4Gen::PCREParser::Character_classContext* ctx);
  [[nodiscard]]
  std::expected<std::string, std::string> decodeEscapeCharacter(const std::string& text);
  [[nodiscard]]
  std::expected<std::string, std::string> decodeEscapeCharacterType(const std::string& text);

private:
  Hir& hir_;
  bool should_visit_next_child_{true};
};
} // namespace Sregex::Parser