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
#include "src/automata/dfa.h"

#include "gtest/gtest.h"

namespace Sregex::Automata {
TEST(DfaTest, Constructor) {
  std::vector<std::string> patterns = {"(你*|好+|(hello)?|(hi){2,4})+世界", "ab12", "ab34",
                                       "[一-龥]"};
  Dfa dfa(patterns);
  EXPECT_TRUE(dfa.ok());
  dfa.toDotSvg("1.svg");
}

TEST(DfaTest, isMatchBacic) {
  // Basic matching
  {
    Dfa dfa("abc");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("abc"));
    EXPECT_TRUE(dfa.isMatch("xabc"));
    EXPECT_TRUE(dfa.isMatch("abcx"));
    EXPECT_TRUE(dfa.isMatch("xabcx"));
    EXPECT_FALSE(dfa.isMatch("a"));
    EXPECT_FALSE(dfa.isMatch("ab"));
    EXPECT_FALSE(dfa.isMatch("abd"));
  }

  // Character class
  {
    Dfa dfa("[abc]+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("abc"));
    EXPECT_TRUE(dfa.isMatch("cba"));
    EXPECT_TRUE(dfa.isMatch("a"));
    EXPECT_FALSE(dfa.isMatch("xyz"));
  }

  // Negated character class
  {
    Dfa dfa("[^abc]+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("xyz"));
    EXPECT_TRUE(dfa.isMatch("hello"));
    EXPECT_FALSE(dfa.isMatch("abc"));
  }

  // Ranges
  {
    Dfa dfa("[a-z]+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("hello"));
    EXPECT_TRUE(dfa.isMatch("abcxyz"));
    EXPECT_FALSE(dfa.isMatch("ABC"));
    EXPECT_FALSE(dfa.isMatch("123"));
  }

  // Hyphens in character classes
  {
    Dfa dfa("[0-9-]+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("12-34"));
    EXPECT_TRUE(dfa.isMatch("0"));
    EXPECT_FALSE(dfa.isMatch("abc"));
  }

  // Dot character class
  {
    Dfa dfa("[.]+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("..."));
    EXPECT_TRUE(dfa.isMatch("."));
    EXPECT_FALSE(dfa.isMatch("abc"));
  }
}

TEST(DfaTest, isMatchWithQuantifiers) {
  // Kleene star
  {
    Dfa dfa("ab*");
    dfa.toDotSvg("1.svg");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("a"));
    EXPECT_TRUE(dfa.isMatch("ab"));
    EXPECT_TRUE(dfa.isMatch("abbb"));
    EXPECT_FALSE(dfa.isMatch("b"));
  }

  // Kleene plus
  {
    Dfa dfa("ab+");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("ab"));
    EXPECT_TRUE(dfa.isMatch("abbb"));
    EXPECT_FALSE(dfa.isMatch("a"));
  }

  // Kleene question mark
  {
    Dfa dfa("ab?c");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("ac"));
    EXPECT_TRUE(dfa.isMatch("abc"));
    EXPECT_FALSE(dfa.isMatch("abbc"));
  }

  // Exact repetition
  {
    Dfa dfa("a{3}");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("aaa"));
    EXPECT_FALSE(dfa.isMatch("aa"));
    EXPECT_FALSE(dfa.isMatch("aab"));
  }

  // At least n repetitions
  {
    Dfa dfa("a{2,}");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("aa"));
    EXPECT_TRUE(dfa.isMatch("aaaaa"));
    EXPECT_FALSE(dfa.isMatch("a"));
  }

  // Bounded repetition
  {
    Dfa dfa("a{2,4}");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("aa"));
    EXPECT_TRUE(dfa.isMatch("aaa"));
    EXPECT_TRUE(dfa.isMatch("aaaa"));
    EXPECT_FALSE(dfa.isMatch("a"));
  }
}

TEST(DfaTest, isMatchWithAlternation) {
  {
    Dfa dfa("cat|dog|elephant");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("cat"));
    EXPECT_TRUE(dfa.isMatch("dog"));
    EXPECT_TRUE(dfa.isMatch("elephant"));
    EXPECT_FALSE(dfa.isMatch("bird"));
  }

  {
    Dfa dfa("(ab|cd)e");
    ASSERT_TRUE(dfa.ok());
    EXPECT_TRUE(dfa.isMatch("abe"));
    EXPECT_TRUE(dfa.isMatch("cde"));
    EXPECT_FALSE(dfa.isMatch("ade"));
    EXPECT_FALSE(dfa.isMatch("cbe"));
  }
}
} // namespace Sregex::Automata
