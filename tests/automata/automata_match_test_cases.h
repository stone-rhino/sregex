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

#include <string>
#include <vector>

#include "gtest/gtest.h"

namespace Sregex::Automata {

template <typename Automaton> class CommonAutomataMatchTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(CommonAutomataMatchTest);

TYPED_TEST_P(CommonAutomataMatchTest, Constructor) {
  std::vector<std::string> patterns = {"(你*|好+|(hello)?|(hi){2,4})+世界", "ab12", "ab34",
                                       "[一-龥]"};
  TypeParam automaton(patterns);
  EXPECT_TRUE(automaton.ok());
  automaton.toDotSvg("1.svg");
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchBasic) {
  // Basic matching
  {
    TypeParam automaton("abc");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_TRUE(automaton.isMatch("xabc"));
    EXPECT_TRUE(automaton.isMatch("abcx"));
    EXPECT_TRUE(automaton.isMatch("xabcx"));
    EXPECT_FALSE(automaton.isMatch("a"));
    EXPECT_FALSE(automaton.isMatch("ab"));
    EXPECT_FALSE(automaton.isMatch("abd"));
  }

  // Character class
  {
    TypeParam automaton("[abc]+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_TRUE(automaton.isMatch("cba"));
    EXPECT_TRUE(automaton.isMatch("a"));
    EXPECT_FALSE(automaton.isMatch("xyz"));
  }

  // Negated character class
  {
    TypeParam automaton("[^abc]+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("xyz"));
    EXPECT_TRUE(automaton.isMatch("hello"));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }

  // Ranges
  {
    TypeParam automaton("[a-z]+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("hello"));
    EXPECT_TRUE(automaton.isMatch("abcxyz"));
    EXPECT_FALSE(automaton.isMatch("ABC"));
    EXPECT_FALSE(automaton.isMatch("123"));
  }

  // Hyphens in character classes
  {
    TypeParam automaton("[0-9-]+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("12-34"));
    EXPECT_TRUE(automaton.isMatch("0"));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }

  // Dot character class
  {
    TypeParam automaton("[.]+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("..."));
    EXPECT_TRUE(automaton.isMatch("."));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchWithQuantifiers) {
  // Kleene star
  {
    TypeParam automaton("ab*");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("a"));
    EXPECT_TRUE(automaton.isMatch("ab"));
    EXPECT_TRUE(automaton.isMatch("abbb"));
    EXPECT_FALSE(automaton.isMatch("b"));
  }

  // Kleene plus
  {
    TypeParam automaton("ab+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("ab"));
    EXPECT_TRUE(automaton.isMatch("abbb"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }

  // Kleene question mark
  {
    TypeParam automaton("ab?c");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("ac"));
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_FALSE(automaton.isMatch("abbc"));
  }

  // Exact repetition
  {
    TypeParam automaton("a{3}");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aaa"));
    EXPECT_FALSE(automaton.isMatch("aa"));
    EXPECT_FALSE(automaton.isMatch("aab"));
  }

  // At least n repetitions
  {
    TypeParam automaton("a{2,}");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aa"));
    EXPECT_TRUE(automaton.isMatch("aaaaa"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }

  // Bounded repetition
  {
    TypeParam automaton("a{2,4}");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aa"));
    EXPECT_TRUE(automaton.isMatch("aaa"));
    EXPECT_TRUE(automaton.isMatch("aaaa"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchWithAlternation) {
  {
    TypeParam automaton("cat|dog|elephant");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("cat"));
    EXPECT_TRUE(automaton.isMatch("dog"));
    EXPECT_TRUE(automaton.isMatch("elephant"));
    EXPECT_FALSE(automaton.isMatch("bird"));
  }

  {
    TypeParam automaton("(ab|cd)e");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abe"));
    EXPECT_TRUE(automaton.isMatch("cde"));
    EXPECT_FALSE(automaton.isMatch("ade"));
    EXPECT_FALSE(automaton.isMatch("cbe"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, LongestPath) {
  {
    TypeParam automaton("(a|1)+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 1);
  }

  {
    TypeParam automaton("(ab|1234)+");
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 4);
  }

  {
    TypeParam automaton("(你*|好+|(hello)?|(hi){2,4})+世界");
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 14);
  }

  {
    TypeParam automaton(
        "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}");
    automaton.toDotSvg("1.svg");
    ASSERT_TRUE(automaton.ok());
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsPartialMatch) {
  // Quantifier prefix
  {
    TypeParam automaton("a{3,5}b");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isPartialMatch("a"));
    EXPECT_TRUE(automaton.isPartialMatch("aa"));
    EXPECT_TRUE(automaton.isPartialMatch("aaa"));
    EXPECT_TRUE(automaton.isPartialMatch("aaaa"));
    EXPECT_TRUE(automaton.isPartialMatch("aaaaaa"));

    const std::string padded = std::string(64, 'a') + "aaa";
    EXPECT_TRUE(automaton.isPartialMatch(padded));

    EXPECT_FALSE(automaton.isPartialMatch("aaaab"));
    EXPECT_FALSE(automaton.isMatch("a"));
    EXPECT_FALSE(automaton.isMatch("aa"));
    EXPECT_TRUE(automaton.isMatch("aaaab"));
  }

  // Alternation prefix
  {
    TypeParam automaton("(cat|dog|elephant)s?");
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isPartialMatch("ca"));
    EXPECT_TRUE(automaton.isPartialMatch("do"));
    EXPECT_TRUE(automaton.isPartialMatch("ele"));

    const std::string padded = std::string(64, 'e') + "ele";
    EXPECT_TRUE(automaton.isPartialMatch(padded));

    EXPECT_FALSE(automaton.isMatch("ca"));
    EXPECT_TRUE(automaton.isMatch("cat"));
    EXPECT_TRUE(automaton.isMatch("dogs"));
  }

  // Partial match under anchors
  // {
  //   TypeParam automaton("^hello world$");
  //   ASSERT_TRUE(automaton.ok());
  //   EXPECT_TRUE(automaton.isPartialMatch("hello"));
  //   EXPECT_TRUE(automaton.isPartialMatch("hello wor"));
  //   EXPECT_FALSE(automaton.isMatch("hello"));
  //   EXPECT_TRUE(automaton.isMatch("hello world"));
  // }

  // Assertion prefix
  // {
  //   TypeParam automaton("foo(?=bar)");
  //   ASSERT_TRUE(automaton.ok());
  //   EXPECT_TRUE(automaton.isPartialMatch("foo"));
  //   EXPECT_TRUE(automaton.isPartialMatch("foob"));
  //   EXPECT_FALSE(automaton.isMatch("foo"));
  //   EXPECT_TRUE(automaton.isMatch("foobar"));
  // }
}

REGISTER_TYPED_TEST_SUITE_P(CommonAutomataMatchTest, Constructor, IsMatchBasic,
                            IsMatchWithQuantifiers, IsMatchWithAlternation, LongestPath,
                            IsPartialMatch);

} // namespace Sregex::Automata