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
#include "src/automata/nfa.h"

#include "gtest/gtest.h"

namespace Sregex::Automata {
TEST(NfaTest, Constructor) {
  std::vector<std::string> patterns = {"(你*|好+|(hello)?|(hi){2,4})+世界", "ab12", "ab34",
                                       "[一-龥]"};
  Nfa nfa(patterns);
  EXPECT_TRUE(nfa.ok());
  nfa.toDotSvg("1.svg");
}

TEST(NfaTest, isMatchBacic) {
  // Basic matching
  {
    Nfa nfa("abc");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("abc"));
    EXPECT_TRUE(nfa.isMatch("xabc"));
    EXPECT_TRUE(nfa.isMatch("abcx"));
    EXPECT_TRUE(nfa.isMatch("xabcx"));
    EXPECT_FALSE(nfa.isMatch("a"));
    EXPECT_FALSE(nfa.isMatch("ab"));
    EXPECT_FALSE(nfa.isMatch("abd"));
  }

  // Character class
  {
    Nfa nfa("[abc]+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("abc"));
    EXPECT_TRUE(nfa.isMatch("cba"));
    EXPECT_TRUE(nfa.isMatch("a"));
    EXPECT_FALSE(nfa.isMatch("xyz"));
  }

  // Negated character class
  {
    Nfa nfa("[^abc]+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("xyz"));
    EXPECT_TRUE(nfa.isMatch("hello"));
    EXPECT_FALSE(nfa.isMatch("abc"));
  }

  // Ranges
  {
    Nfa nfa("[a-z]+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("hello"));
    EXPECT_TRUE(nfa.isMatch("abcxyz"));
    EXPECT_FALSE(nfa.isMatch("ABC"));
    EXPECT_FALSE(nfa.isMatch("123"));
  }

  // Hyphens in character classes
  {
    Nfa nfa("[0-9-]+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("12-34"));
    EXPECT_TRUE(nfa.isMatch("0"));
    EXPECT_FALSE(nfa.isMatch("abc"));
  }

  // Dot character class
  {
    Nfa nfa("[.]+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("..."));
    EXPECT_TRUE(nfa.isMatch("."));
    EXPECT_FALSE(nfa.isMatch("abc"));
  }
}

TEST(NfaTest, isMatchWithQuantifiers) {
  // Kleene star
  {
    Nfa nfa("ab*");
    nfa.toDotSvg("1.svg");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("a"));
    EXPECT_TRUE(nfa.isMatch("ab"));
    EXPECT_TRUE(nfa.isMatch("abbb"));
    EXPECT_FALSE(nfa.isMatch("b"));
  }

  // Kleene plus
  {
    Nfa nfa("ab+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("ab"));
    EXPECT_TRUE(nfa.isMatch("abbb"));
    EXPECT_FALSE(nfa.isMatch("a"));
  }

  // Kleene question mark
  {
    Nfa nfa("ab?c");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("ac"));
    EXPECT_TRUE(nfa.isMatch("abc"));
    EXPECT_FALSE(nfa.isMatch("abbc"));
  }

  // Exact repetition
  {
    Nfa nfa("a{3}");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("aaa"));
    EXPECT_FALSE(nfa.isMatch("aa"));
    EXPECT_FALSE(nfa.isMatch("aab"));
  }

  // At least n repetitions
  {
    Nfa nfa("a{2,}");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("aa"));
    EXPECT_TRUE(nfa.isMatch("aaaaa"));
    EXPECT_FALSE(nfa.isMatch("a"));
  }

  // Bounded repetition
  {
    Nfa nfa("a{2,4}");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("aa"));
    EXPECT_TRUE(nfa.isMatch("aaa"));
    EXPECT_TRUE(nfa.isMatch("aaaa"));
    EXPECT_FALSE(nfa.isMatch("a"));
  }
}

TEST(NfaTest, isMatchWithAlternation) {
  {
    Nfa nfa("cat|dog|elephant");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("cat"));
    EXPECT_TRUE(nfa.isMatch("dog"));
    EXPECT_TRUE(nfa.isMatch("elephant"));
    EXPECT_FALSE(nfa.isMatch("bird"));
  }

  {
    Nfa nfa("(ab|cd)e");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isMatch("abe"));
    EXPECT_TRUE(nfa.isMatch("cde"));
    EXPECT_FALSE(nfa.isMatch("ade"));
    EXPECT_FALSE(nfa.isMatch("cbe"));
  }
}

TEST(NfaTest, longestPath) {
  {
    Nfa nfa("(a|1)+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_EQ(nfa.longestPath(), 1);
  }

  {
    Nfa nfa("(ab|1234)+");
    ASSERT_TRUE(nfa.ok());
    EXPECT_EQ(nfa.longestPath(), 4);
  }

  {
    Nfa nfa("(你*|好+|(hello)?|(hi){2,4})+世界");
    ASSERT_TRUE(nfa.ok());
    EXPECT_EQ(nfa.longestPath(), 14);
  }

  {
    Nfa nfa(
        "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}");
    nfa.toDotSvg("1.svg");
    ASSERT_TRUE(nfa.ok());
  }
}

TEST(NfaTest, isPartialMatch) {
  // Quantifier prefix
  {
    Nfa nfa("a{3,5}b");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isPartialMatch("a"));
    EXPECT_TRUE(nfa.isPartialMatch("aa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaaa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaaaa"));
    EXPECT_FALSE(nfa.isPartialMatch("aaaab"));
    EXPECT_FALSE(nfa.isMatch("a"));
    EXPECT_FALSE(nfa.isMatch("aa"));
    EXPECT_TRUE(nfa.isMatch("aaaab"));
  }

  // Alternation prefix
  {
    Nfa nfa("(cat|dog|elephant)s?");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isPartialMatch("ca"));
    EXPECT_TRUE(nfa.isPartialMatch("do"));
    EXPECT_TRUE(nfa.isPartialMatch("ele"));
    EXPECT_FALSE(nfa.isMatch("ca"));
    EXPECT_TRUE(nfa.isMatch("cat"));
    EXPECT_TRUE(nfa.isMatch("dogs"));
  }

  // Partial match under anchors
  // {
  //   Nfa nfa("^hello world$");
  //   ASSERT_TRUE(nfa.ok());
  //   EXPECT_TRUE(nfa.isPartialMatch("hello"));
  //   EXPECT_TRUE(nfa.isPartialMatch("hello wor"));
  //   EXPECT_FALSE(nfa.isMatch("hello"));
  //   EXPECT_TRUE(nfa.isMatch("hello world"));
  // }

  // Assertion prefix
  // {
  //   Nfa nfa("foo(?=bar)");
  //   ASSERT_TRUE(nfa.ok());
  //   EXPECT_TRUE(nfa.isPartialMatch("foo"));
  //   EXPECT_TRUE(nfa.isPartialMatch("foob"));
  //   EXPECT_FALSE(nfa.isMatch("foo"));
  //   EXPECT_TRUE(nfa.isMatch("foobar"));
  // }
}

} // namespace Sregex::Automata