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
  auto result = Sregex::Parser::Hir::parse("[a-z]");
  if (!result) {
    std::cerr << result.error() << std::endl;
  }
  ASSERT_TRUE(result.has_value());

  auto hir = std::move(result).value();
  ASSERT_TRUE(hir->kind.is<UnicodeClassKind>());
  auto& unicode_class = hir->kind.as<UnicodeClassKind>();
  EXPECT_EQ(unicode_class.ranges_.size(), 1);
  EXPECT_EQ(unicode_class.ranges_[0].start_, 'a');
  EXPECT_EQ(unicode_class.ranges_[0].end_, 'z');
}
} // namespace Sregex::Parser