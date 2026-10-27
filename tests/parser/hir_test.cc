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
#include <iostream>

#include "gtest/gtest.h"
#include "parser/hir.h"

namespace Sregex::Parser {
TEST(HirTest, OptionSettings) {
  {
    auto result = Hir::parse("");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 0);
  }

  {
    auto result = Hir::parse("(*UTF)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 1);
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UTF8)));
  }

  {
    auto result = Hir::parse("(*UTF8)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 1);
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UTF8)));
  }

  {
    auto result = Hir::parse("(*UTF16)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 1);
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UTF16)));
  }

  {
    auto result = Hir::parse("(*UTF32)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 1);
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UTF32)));
  }

  {
    auto result = Hir::parse("(*UTF)(*UCP)(*NO_AUTO_POSSESS)(*NO_START_OPT)(*CR)(*LF)(*CRLF)(*"
                             "ANYCRLF)(*ANY)(*BSR_ANYCRLF)(*BSR_UNICODE)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 11);
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UTF8)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::UCP)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NoAutoPossess)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NoStartOpt)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NewlineCR)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NewlineLF)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NewlineCRLF)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NewlineAnyCRLF)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::NewlineAny)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::BsrAnyCRLF)));
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::BsrUnicode)));
  }

  {
    auto result = Hir::parse("(?i)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 1);
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
  }

  {
    auto result = Hir::parse("(?i)(?-i)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 0);
    EXPECT_FALSE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
  }

  {
    auto result = Hir::parse("(?iJmsUx)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 6);
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
    EXPECT_TRUE(hir->option_settings_.test(
        static_cast<size_t>(OptionSettingBitOffset::DuplicateNamedCaptureGroups)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Multiline)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::DotAll)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Ungreedy)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Extended)));
  }

  {
    auto result = Hir::parse("(?iJmsUx-iJm)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    EXPECT_EQ(hir->option_settings_.count(), 3);
    EXPECT_FALSE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
    EXPECT_FALSE(hir->option_settings_.test(
        static_cast<size_t>(OptionSettingBitOffset::DuplicateNamedCaptureGroups)));
    EXPECT_FALSE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Multiline)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::DotAll)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Ungreedy)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Extended)));
  }
}

TEST(HirTest, EmptyKind) {
  auto result = Hir::parse("");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<EmptyKind>());
  EXPECT_EQ(hir->dump(), "empty");
}

TEST(HirTest, LiteralKind) {
  auto result = Hir::parse("你好世界helloworld123\\a");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<LiteralKind>());
  EXPECT_EQ(hir->kind.as<LiteralKind>(), "你好世界helloworld123\a");
  EXPECT_EQ(hir->dump(), "lit{你好世界helloworld123\a}");
}

TEST(HirTest, LiteralKindWithEscape) {
  {
    auto result = Hir::parse(R"(\Q.+?*\E)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<LiteralKind>());
    EXPECT_EQ(hir->kind.as<LiteralKind>(), ".+?*");
    EXPECT_EQ(hir->dump(), "lit{.+?*}");
  }

  {
    auto result = Hir::parse(R"(\.\+\?\*)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<LiteralKind>());
    EXPECT_EQ(hir->kind.as<LiteralKind>(), ".+?*");
    EXPECT_EQ(hir->dump(), "lit{.+?*}");
  }
}

TEST(HirTest, UnicodeClassKind) {
  auto result = Sregex::Parser::Hir::parse("[.一-三0-27-91-6[:alpha:]]");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<UnicodeClassKind>());
  const auto& unicode_class = hir->kind.as<UnicodeClassKind>();
  ASSERT_EQ(unicode_class.ranges_.size(), 5);
  EXPECT_EQ(unicode_class.ranges_[0].start_, '.');
  EXPECT_EQ(unicode_class.ranges_[0].end_, '.');
  EXPECT_EQ(unicode_class.ranges_[1].start_, '0');
  EXPECT_EQ(unicode_class.ranges_[1].end_, '9');
  EXPECT_EQ(unicode_class.ranges_[2].start_, 'A');
  EXPECT_EQ(unicode_class.ranges_[2].end_, 'Z');
  EXPECT_EQ(unicode_class.ranges_[3].start_, 'a');
  EXPECT_EQ(unicode_class.ranges_[3].end_, 'z');
  EXPECT_EQ(unicode_class.ranges_[4].start_, U'一');
  EXPECT_EQ(unicode_class.ranges_[4].end_, U'三');
  EXPECT_EQ(hir->dump(), "uc{0x2e 0x30-0x39 0x41-0x5a 0x61-0x7a 0x4e00-0x4e09}");

  // Test negated unicode class
  {
    auto result = Sregex::Parser::Hir::parse("[^一-三\\d[:alpha:]]");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<UnicodeClassKind>());
    const auto& unicode_class = hir->kind.as<UnicodeClassKind>();
    ASSERT_EQ(unicode_class.ranges_.size(), 5);
    EXPECT_EQ(unicode_class.ranges_[0].start_, 0);
    EXPECT_EQ(unicode_class.ranges_[0].end_, '0' - 1);
    EXPECT_EQ(unicode_class.ranges_[1].start_, '9' + 1);
    EXPECT_EQ(unicode_class.ranges_[1].end_, 'A' - 1);
    EXPECT_EQ(unicode_class.ranges_[2].start_, 'Z' + 1);
    EXPECT_EQ(unicode_class.ranges_[2].end_, 'a' - 1);
    EXPECT_EQ(unicode_class.ranges_[3].start_, 'z' + 1);
    EXPECT_EQ(unicode_class.ranges_[3].end_, U'一' - 1);
    EXPECT_EQ(unicode_class.ranges_[4].start_, U'三' + 1);
    EXPECT_EQ(unicode_class.ranges_[4].end_, 0x10ffff);
    EXPECT_EQ(hir->dump(), "uc{0x00-0x2f 0x3a-0x40 0x5b-0x60 0x7b-0x4dff 0x4e0a-0x10ffff}");
  }
}

TEST(HirTest, AnchorKind) {
  auto result = Sregex::Parser::Hir::parse(R"(^$\b\B\A\z\Z\G)");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<ConcatKind>());
  const auto& concat = hir->kind.as<ConcatKind>();
  ASSERT_EQ(concat.size(), 8);
  ASSERT_TRUE(concat[0]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[0]->kind.as<AnchorKind>(), AnchorKind::Start);
  ASSERT_TRUE(concat[1]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[1]->kind.as<AnchorKind>(), AnchorKind::End);
  ASSERT_TRUE(concat[2]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[2]->kind.as<AnchorKind>(), AnchorKind::WordBoundary);
  ASSERT_TRUE(concat[3]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[3]->kind.as<AnchorKind>(), AnchorKind::NonWordBoundary);
  ASSERT_TRUE(concat[4]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[4]->kind.as<AnchorKind>(), AnchorKind::Start);
  ASSERT_TRUE(concat[5]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[5]->kind.as<AnchorKind>(), AnchorKind::End);
  ASSERT_TRUE(concat[6]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[6]->kind.as<AnchorKind>(), AnchorKind::EndLine);
  ASSERT_TRUE(concat[7]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[7]->kind.as<AnchorKind>(), AnchorKind::LastMatchPosition);

  // clang-format off
  EXPECT_EQ(hir->dump(), "cat{anc{Start} anc{End} anc{WordBoundary} anc{NonWordBoundary} anc{Start} anc{End} anc{EndLine} anc{LastMatchPosition}}");
  // clang-format on
}

TEST(HirTest, RepetitionKindWithGreedy) {
  {
    auto result = Sregex::Parser::Hir::parse("a{3,9}");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, 9);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,9 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a{3,}");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a{3}");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, 3);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,3 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, 1);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a+");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 1);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{1,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a*");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("ab*");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<ConcatKind>());
    const auto& concat = hir->kind.as<ConcatKind>();

    const auto& literal = concat[0]->kind.as<LiteralKind>();
    EXPECT_EQ(literal, "a");

    const auto& rep = concat[1]->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "b");
    EXPECT_EQ(hir->dump(), "cat{lit{a} rep{0,-1 lit{b}}}");
  }
}

TEST(HirTest, RepetitionKindWithLazy) {
  {
    auto result = Sregex::Parser::Hir::parse("a{3,9}?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, 9);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,9 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a{3,}?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a??");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, 1);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a+?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 1);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{1,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a*?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("ab*?");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<ConcatKind>());
    const auto& concat = hir->kind.as<ConcatKind>();

    const auto& literal = concat[0]->kind.as<LiteralKind>();
    EXPECT_EQ(literal, "a");

    const auto& rep = concat[1]->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_FALSE(rep.greedy_);
    EXPECT_FALSE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "b");
    EXPECT_EQ(hir->dump(), "cat{lit{a} rep{0,-1 lit{b}}}");
  }
}

TEST(HirTest, RepetitionKindWithPossessive) {
  {
    auto result = Sregex::Parser::Hir::parse("a{3,9}+");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, 9);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_TRUE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,9 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a{3,}+");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 3);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_TRUE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{3,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a?+");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, 1);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_TRUE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a++");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 1);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_TRUE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{1,-1 lit{a}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("a*+");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<RepetitionKind>());
    const auto& rep = hir->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 0);
    EXPECT_EQ(rep.max_, std::nullopt);
    EXPECT_TRUE(rep.greedy_);
    EXPECT_TRUE(rep.possessive_);
    EXPECT_TRUE(rep.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(rep.sub_->kind.as<LiteralKind>(), "a");
    EXPECT_EQ(hir->dump(), "rep{0,-1 lit{a}}");
  }
}

TEST(HirTest, CaptureKind) {
  {
    auto result = Sregex::Parser::Hir::parse("(?<hi>helloworld)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<CaptureKind>());
    const auto& cap = hir->kind.as<CaptureKind>();
    EXPECT_EQ(cap.index_, 0);
    EXPECT_EQ(cap.name_, "hi");
    ASSERT_TRUE(cap.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(cap.sub_->kind.as<LiteralKind>(), "helloworld");
    EXPECT_EQ(hir->dump(), "cap{lit{helloworld}}");
  }

  {
    auto result = Sregex::Parser::Hir::parse("(?im:helloworld)");
    if (!result) {
      std::cerr << result.error() << std::endl;
    }
    ASSERT_TRUE(result.has_value());

    auto hir = std::move(result).value();
    ASSERT_TRUE(hir->kind.is<CaptureKind>());
    const auto& cap = hir->kind.as<CaptureKind>();
    EXPECT_EQ(cap.index_, 0);
    EXPECT_EQ(cap.name_, "");
    EXPECT_TRUE(
        hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
    EXPECT_TRUE(hir->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Multiline)));
    ASSERT_TRUE(cap.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(cap.sub_->kind.as<LiteralKind>(), "helloworld");
    EXPECT_TRUE(cap.sub_->option_settings_.test(
        static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
    EXPECT_TRUE(
        cap.sub_->option_settings_.test(static_cast<size_t>(OptionSettingBitOffset::Multiline)));
    EXPECT_EQ(hir->dump(), "cap{lit{helloworld}}");
  }
}

TEST(HirTest, ConcatKind) {
  auto result = Hir::parse("^hello[wW](?i:orld)[一-龥]+$");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<ConcatKind>());
  const auto& concat = hir->kind.as<ConcatKind>();
  ASSERT_EQ(concat.size(), 6);

  ASSERT_TRUE(concat[0]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[0]->kind.as<AnchorKind>(), AnchorKind::Start);

  ASSERT_TRUE(concat[1]->kind.is<LiteralKind>());
  EXPECT_EQ(concat[1]->kind.as<LiteralKind>(), "hello");

  ASSERT_TRUE(concat[2]->kind.is<UnicodeClassKind>());
  const auto& unicode_class = concat[2]->kind.as<UnicodeClassKind>();
  ASSERT_EQ(unicode_class.ranges_.size(), 2);
  EXPECT_EQ(unicode_class.ranges_[0].start_, 'W');
  EXPECT_EQ(unicode_class.ranges_[0].end_, 'W');
  EXPECT_EQ(unicode_class.ranges_[1].start_, 'w');
  EXPECT_EQ(unicode_class.ranges_[1].end_, 'w');

  EXPECT_TRUE(concat[3]->option_settings_.test(
      static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
  ASSERT_TRUE(concat[3]->kind.is<CaptureKind>());
  const auto& cap = concat[3]->kind.as<CaptureKind>();
  ASSERT_TRUE(cap.sub_);
  ASSERT_TRUE(cap.sub_->kind.is<LiteralKind>());
  EXPECT_EQ(cap.sub_->kind.as<LiteralKind>(), "orld");
  EXPECT_TRUE(cap.sub_->option_settings_.test(
      static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));

  ASSERT_TRUE(concat[4]->kind.is<RepetitionKind>());
  const auto& rep = concat[4]->kind.as<RepetitionKind>();
  EXPECT_EQ(rep.min_, 1);
  EXPECT_EQ(rep.max_, std::nullopt);
  ASSERT_TRUE(rep.sub_);
  ASSERT_TRUE(rep.sub_->kind.is<UnicodeClassKind>());
  const auto& unicode_class2 = rep.sub_->kind.as<UnicodeClassKind>();
  ASSERT_EQ(unicode_class2.ranges_.size(), 1);
  EXPECT_EQ(unicode_class2.ranges_[0].start_, U'一');
  EXPECT_EQ(unicode_class2.ranges_[0].end_, U'龥');

  ASSERT_TRUE(concat[5]->kind.is<AnchorKind>());
  EXPECT_EQ(concat[5]->kind.as<AnchorKind>(), AnchorKind::End);
  EXPECT_EQ(hir->dump(), "cat{anc{Start} lit{hello} uc{0x57 0x77} cap{lit{orld}} rep{1,-1 "
                         "uc{0x4e00-0x9fa5}} anc{End}}");
}

TEST(HirTest, AlternationKind) {
  auto result = Hir::parse("^hello[wW](?i:orld)[一-龥]+$|foo|bar");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<AlternationKind>());
  const auto& alt = hir->kind.as<AlternationKind>();
  ASSERT_EQ(alt.size(), 3);

  // Test the first alternative in the alternation (same as ^hello[wW](?i:orld)[一-龥]+$ in
  // ConcatKind test case)
  {
    ASSERT_TRUE(alt[0]->kind.is<ConcatKind>());
    const auto& concat = alt[0]->kind.as<ConcatKind>();
    ASSERT_TRUE(concat[0]->kind.is<AnchorKind>());
    EXPECT_EQ(concat[0]->kind.as<AnchorKind>(), AnchorKind::Start);

    ASSERT_TRUE(concat[1]->kind.is<LiteralKind>());
    EXPECT_EQ(concat[1]->kind.as<LiteralKind>(), "hello");

    ASSERT_TRUE(concat[2]->kind.is<UnicodeClassKind>());
    const auto& unicode_class = concat[2]->kind.as<UnicodeClassKind>();
    ASSERT_EQ(unicode_class.ranges_.size(), 2);
    EXPECT_EQ(unicode_class.ranges_[0].start_, 'W');
    EXPECT_EQ(unicode_class.ranges_[0].end_, 'W');
    EXPECT_EQ(unicode_class.ranges_[1].start_, 'w');
    EXPECT_EQ(unicode_class.ranges_[1].end_, 'w');

    EXPECT_TRUE(concat[3]->option_settings_.test(
        static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));
    ASSERT_TRUE(concat[3]->kind.is<CaptureKind>());
    const auto& cap = concat[3]->kind.as<CaptureKind>();
    ASSERT_TRUE(cap.sub_);
    ASSERT_TRUE(cap.sub_->kind.is<LiteralKind>());
    EXPECT_EQ(cap.sub_->kind.as<LiteralKind>(), "orld");
    EXPECT_TRUE(cap.sub_->option_settings_.test(
        static_cast<size_t>(OptionSettingBitOffset::CaseInsensitive)));

    ASSERT_TRUE(concat[4]->kind.is<RepetitionKind>());
    const auto& rep = concat[4]->kind.as<RepetitionKind>();
    EXPECT_EQ(rep.min_, 1);
    EXPECT_EQ(rep.max_, std::nullopt);
    ASSERT_TRUE(rep.sub_);
    ASSERT_TRUE(rep.sub_->kind.is<UnicodeClassKind>());
    const auto& unicode_class2 = rep.sub_->kind.as<UnicodeClassKind>();
    ASSERT_EQ(unicode_class2.ranges_.size(), 1);
    EXPECT_EQ(unicode_class2.ranges_[0].start_, U'一');
    EXPECT_EQ(unicode_class2.ranges_[0].end_, U'龥');

    ASSERT_TRUE(concat[5]->kind.is<AnchorKind>());
    EXPECT_EQ(concat[5]->kind.as<AnchorKind>(), AnchorKind::End);
  }

  // Test the second alternative in the alternation (foo)
  {
    ASSERT_TRUE(alt[1]->kind.is<LiteralKind>());
    EXPECT_EQ(alt[1]->kind.as<LiteralKind>(), "foo");
  }

  // Test the third alternative in the alternation (bar)
  {
    ASSERT_TRUE(alt[2]->kind.is<LiteralKind>());
    EXPECT_EQ(alt[2]->kind.as<LiteralKind>(), "bar");
  }

  // clang-format off
  EXPECT_EQ(hir->dump(), "alt{cat{anc{Start} lit{hello} uc{0x57 0x77} cap{lit{orld}} rep{1,-1 uc{0x4e00-0x9fa5}} anc{End}} | lit{foo} | lit{bar}}");
  // clang-format on
}

} // namespace Sregex::Parser