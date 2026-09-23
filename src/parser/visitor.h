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
#include <stack>
#include <vector>

#include "antlr4_gen/PCREBaseVisitor.h"
#include "hir_kind.h"

namespace Sregex::Parser {
class Hir;
class Visitor final : public Antlr4Gen::PCREBaseVisitor {
public:
  Visitor(Hir& hir);

protected:
  std::any visitPcre(Antlr4Gen::PCREParser::PcreContext* ctx) override;

  std::any visitAlternation(Antlr4Gen::PCREParser::AlternationContext* ctx) override;

  std::any visitExpr(Antlr4Gen::PCREParser::ExprContext* ctx) override;

  std::any visitOption_setting(Antlr4Gen::PCREParser::Option_settingContext* ctx) override;

  std::any
  visitBacktracking_control(Antlr4Gen::PCREParser::Backtracking_controlContext* ctx) override;

  std::any visitCallout(Antlr4Gen::PCREParser::CalloutContext* ctx) override;

  std::any visitCapture(Antlr4Gen::PCREParser::CaptureContext* ctx) override;

  std::any visitAtomic_group(Antlr4Gen::PCREParser::Atomic_groupContext* ctx) override;

  std::any visitLookaround(Antlr4Gen::PCREParser::LookaroundContext* ctx) override;

  std::any visitBackreference(Antlr4Gen::PCREParser::BackreferenceContext* ctx) override;

  std::any
  visitSubroutine_reference(Antlr4Gen::PCREParser::Subroutine_referenceContext* ctx) override;

  std::any
  visitConditional_pattern(Antlr4Gen::PCREParser::Conditional_patternContext* ctx) override;

  std::any visitComment(Antlr4Gen::PCREParser::CommentContext* ctx) override;

  std::any visitQuantifier(Antlr4Gen::PCREParser::QuantifierContext* ctx) override;

  std::any visitCharacter(Antlr4Gen::PCREParser::CharacterContext* ctx) override;

  std::any visitCharacter_type(Antlr4Gen::PCREParser::Character_typeContext* ctx) override;

  std::any visitCharacter_class(Antlr4Gen::PCREParser::Character_classContext* ctx) override;

  std::any
  visitCharacter_class_atom(Antlr4Gen::PCREParser::Character_class_atomContext* ctx) override;

  std::any
  visitPosix_character_class(Antlr4Gen::PCREParser::Posix_character_classContext* ctx) override;

  std::any visitLetter(Antlr4Gen::PCREParser::LetterContext* ctx) override;

  std::any visitDigit(Antlr4Gen::PCREParser::DigitContext* ctx) override;

  std::any visitAnchor(Antlr4Gen::PCREParser::AnchorContext* ctx) override;

  std::any visitMatch_point_reset(Antlr4Gen::PCREParser::Match_point_resetContext* ctx) override;

  std::any visitQuoting(Antlr4Gen::PCREParser::QuotingContext* ctx) override;

  std::any visitOther(Antlr4Gen::PCREParser::OtherContext* ctx) override;

  bool shouldVisitNextChild(antlr4::tree::ParseTree*, const std::any&) override {
    return should_visit_next_child_;
  }

private:
  // Decode escape characters, including \a, \cX, \e, \f, \n, \r, \t, and numeric escapes
  [[nodiscard]]
  static std::expected<std::string, std::string> decodeEscapeCharacter(const std::string& text);

  // Decode escape character types, e.g., \d, \w, \s
  [[nodiscard]]
  static std::expected<UnicodeClassKind, std::string>
  decodeEscapeCharacterType(const std::string& text);

  // Merge overlapping or adjacent Unicode class ranges into a single range
  // e.g., mergeUnicodeClassRanges([{'a', 'e'}, {'f', 'z'}]) => [{'a', 'z'}]
  static void mergeUnicodeClassRanges(UnicodeClassKind& class_kind);

  // Get POSIX character classes. e.g., [:alnum:], [:alpha:], [:digit:]
  [[nodiscard]]
  static std::expected<UnicodeClassKind, std::string>
  getPosixCharacterClass(const std::string& name, bool negate);

  // Create a negated version of the given Unicode class
  static UnicodeClassKind makeNegated(const UnicodeClassKind& class_kind);

  // Convert a UTF-8 encoded string to a UTF-32 encoded string
  static std::u32string utf8ToUtf32(const std::string& text);

  // Flatten the HIR tree by merging nested concatenations and alternations into a single level
  static void flatten(Hir* hir);

  // Append a literal string to the given HIR node
  static void appendLiteral(Hir* parent, const std::string& text);

  // Add a child HIR node to the given parent HIR node
  static void addHirNode(Hir* parent, std::unique_ptr<Hir>&& child);

private:
  Hir& hir_;
  bool should_visit_next_child_{true};
  std::stack<Hir*> stack_;
};
} // namespace Sregex::Parser