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
TEST(HirTest, EmptyKind) {
  auto result = Hir::parse("");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<EmptyKind>());
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
}

TEST(HirTest, UnicodeClassKind) {
  auto result = Sregex::Parser::Hir::parse("[一-三0-27-91-6[:alpha:]]");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<UnicodeClassKind>());
  const auto& unicode_class = hir->kind.as<UnicodeClassKind>();
  ASSERT_EQ(unicode_class.ranges_.size(), 4);
  EXPECT_EQ(unicode_class.ranges_[0].start_, '0');
  EXPECT_EQ(unicode_class.ranges_[0].end_, '9');
  EXPECT_EQ(unicode_class.ranges_[1].start_, 'A');
  EXPECT_EQ(unicode_class.ranges_[1].end_, 'Z');
  EXPECT_EQ(unicode_class.ranges_[2].start_, 'a');
  EXPECT_EQ(unicode_class.ranges_[2].end_, 'z');
  EXPECT_EQ(unicode_class.ranges_[3].start_, U'一');
  EXPECT_EQ(unicode_class.ranges_[3].end_, U'三');

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
  ASSERT_TRUE(concat[0]->is<AnchorKind>());
  EXPECT_EQ(concat[0]->as<AnchorKind>(), AnchorKind::Start);
  ASSERT_TRUE(concat[1]->is<AnchorKind>());
  EXPECT_EQ(concat[1]->as<AnchorKind>(), AnchorKind::End);
  ASSERT_TRUE(concat[2]->is<AnchorKind>());
  EXPECT_EQ(concat[2]->as<AnchorKind>(), AnchorKind::WordBoundary);
  ASSERT_TRUE(concat[3]->is<AnchorKind>());
  EXPECT_EQ(concat[3]->as<AnchorKind>(), AnchorKind::NonWordBoundary);
  ASSERT_TRUE(concat[4]->is<AnchorKind>());
  EXPECT_EQ(concat[4]->as<AnchorKind>(), AnchorKind::Start);
  ASSERT_TRUE(concat[5]->is<AnchorKind>());
  EXPECT_EQ(concat[5]->as<AnchorKind>(), AnchorKind::End);
  ASSERT_TRUE(concat[6]->is<AnchorKind>());
  EXPECT_EQ(concat[6]->as<AnchorKind>(), AnchorKind::EndLine);
  ASSERT_TRUE(concat[7]->is<AnchorKind>());
  EXPECT_EQ(concat[7]->as<AnchorKind>(), AnchorKind::LastMatchPosition);
}

} // namespace Sregex::Parser