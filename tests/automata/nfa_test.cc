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
#include "gtest/gtest.h"
#include "src/automata/nfa.h"
#include "tests/automata/automata_match_test_cases.h"

namespace Sregex::Automata {
using NfaTypes = ::testing::Types<Nfa>;
INSTANTIATE_TYPED_TEST_SUITE_P(NfaTest, CommonAutomataMatchTest, NfaTypes);

TEST(NfaTest, isPartialMatch) {
  // Quantifier prefix
  {
    Nfa nfa("a{3,5}b");
    ASSERT_TRUE(nfa.ok());
    EXPECT_TRUE(nfa.isPartialMatch("a"));
    EXPECT_TRUE(nfa.isPartialMatch("aa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaaa"));
    EXPECT_TRUE(nfa.isPartialMatch("aaaaaa"));

    const std::string padded = std::string(64, 'a') + "aaa";
    EXPECT_TRUE(nfa.isPartialMatch(padded));

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

    const std::string padded = std::string(64, 'e') + "ele";
    EXPECT_TRUE(nfa.isPartialMatch(padded));

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