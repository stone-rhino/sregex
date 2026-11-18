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
#include "src/sregex.h"

namespace Sregex::Automata {

template <typename Automaton> class CommonAutomataMatchTest : public ::testing::Test {};

TYPED_TEST_SUITE_P(CommonAutomataMatchTest);

TYPED_TEST_P(CommonAutomataMatchTest, Constructor) {
  std::vector<std::string> patterns = {"(你*|好+|(hello)?|(hi){2,4})+世界", "ab12", "ab34",
                                       "[一-龥]"};
  TypeParam automaton(patterns, PatternKind::Regex);
  EXPECT_TRUE(automaton.ok());

  // Test empty patterns
  {
    patterns.clear();
    TypeParam automaton(patterns, PatternKind::Regex);
    EXPECT_FALSE(automaton.ok());
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, Serialization) {
  std::vector<std::string> patterns = {"(你*|好+|(hello)?|(hi){2,4})+世界", "ab12", "ab34",
                                       "[一-龥]"};
  TypeParam automaton(patterns, PatternKind::Regex);
  EXPECT_TRUE(automaton.ok());
  std::string dot = automaton.toDot();

  const std::string filename = "test_automaton.dat";
  EXPECT_TRUE(automaton.serialize(filename));
  TypeParam deserialized_automaton;
  EXPECT_TRUE(deserialized_automaton.deserialize(filename));
  EXPECT_TRUE(deserialized_automaton.ok());
  std::string deserialized_dot = deserialized_automaton.toDot();
  EXPECT_EQ(dot, deserialized_dot);
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchBasic) {
  // Basic matching
  {
    TypeParam automaton("abc", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_TRUE(automaton.isMatch("xabc"));
    EXPECT_TRUE(automaton.isMatch("abcx"));
    EXPECT_TRUE(automaton.isMatch("xabcx"));
    EXPECT_FALSE(automaton.isMatch("a"));
    EXPECT_FALSE(automaton.isMatch("ab"));
    EXPECT_FALSE(automaton.isMatch("abd"));
  }

  // Anchored match
  {
    TypeParam automaton("abc", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abcx", true));
    EXPECT_FALSE(automaton.isMatch("xabc", true));
    EXPECT_TRUE(automaton.isMatch("abcx", false));
    EXPECT_TRUE(automaton.isMatch("xabc", false));
  }

  // Character class
  {
    TypeParam automaton("[abc]+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_TRUE(automaton.isMatch("cba"));
    EXPECT_TRUE(automaton.isMatch("a"));
    EXPECT_FALSE(automaton.isMatch("xyz"));
  }

  // Negated character class
  {
    TypeParam automaton("[^abc]+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("xyz"));
    EXPECT_TRUE(automaton.isMatch("hello"));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }

  // Ranges
  {
    TypeParam automaton("[a-z]+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("hello"));
    EXPECT_TRUE(automaton.isMatch("abcxyz"));
    EXPECT_FALSE(automaton.isMatch("ABC"));
    EXPECT_FALSE(automaton.isMatch("123"));
  }

  // Hyphens in character classes
  {
    TypeParam automaton("[0-9-]+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("12-34"));
    EXPECT_TRUE(automaton.isMatch("0"));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }

  // Dot character class
  {
    TypeParam automaton("[.]+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("..."));
    EXPECT_TRUE(automaton.isMatch("."));
    EXPECT_FALSE(automaton.isMatch("abc"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchWithQuantifiers) {
  // Kleene star
  {
    TypeParam automaton("ab*", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("a"));
    EXPECT_TRUE(automaton.isMatch("ab"));
    EXPECT_TRUE(automaton.isMatch("abbb"));
    EXPECT_FALSE(automaton.isMatch("b"));
  }

  // Kleene plus
  {
    TypeParam automaton("ab+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("ab"));
    EXPECT_TRUE(automaton.isMatch("abbb"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }

  // Kleene question mark
  {
    TypeParam automaton("ab?c", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("ac"));
    EXPECT_TRUE(automaton.isMatch("abc"));
    EXPECT_FALSE(automaton.isMatch("abbc"));
  }

  // Exact repetition
  {
    TypeParam automaton("a{3}", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aaa"));
    EXPECT_FALSE(automaton.isMatch("aa"));
    EXPECT_FALSE(automaton.isMatch("aab"));
  }

  // At least n repetitions
  {
    TypeParam automaton("a{2,}", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aa"));
    EXPECT_TRUE(automaton.isMatch("aaaaa"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }

  // Bounded repetition
  {
    TypeParam automaton("a{2,4}", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("aa"));
    EXPECT_TRUE(automaton.isMatch("aaa"));
    EXPECT_TRUE(automaton.isMatch("aaaa"));
    EXPECT_FALSE(automaton.isMatch("a"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsMatchWithAlternation) {
  {
    TypeParam automaton("cat|dog|elephant", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("cat"));
    EXPECT_TRUE(automaton.isMatch("dog"));
    EXPECT_TRUE(automaton.isMatch("elephant"));
    EXPECT_FALSE(automaton.isMatch("bird"));
  }

  {
    TypeParam automaton("(ab|cd)e", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isMatch("abe"));
    EXPECT_TRUE(automaton.isMatch("cde"));
    EXPECT_FALSE(automaton.isMatch("ade"));
    EXPECT_FALSE(automaton.isMatch("cbe"));
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, LongestPath) {
  {
    TypeParam automaton("(a|1)+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 1);
  }

  {
    TypeParam automaton("(ab|1234)+", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 4);
  }

  {
    TypeParam automaton("(你*|好+|(hello)?|(hi){2,4})+世界", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_EQ(automaton.longestPath(), 14);
  }

  {
    TypeParam automaton(
        "[A-Za-z0-9._%+-]{1,15}@[A-Za-z0-9-]{1,15}([.][A-Za-z0-9-]{1,15}){1,3}[.][A-Za-z]{2,8}",
        PatternKind::Regex);
    automaton.toDotSvg("1.svg");
    ASSERT_TRUE(automaton.ok());
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, IsPartialMatch) {
  // Quantifier prefix
  {
    TypeParam automaton("a{3,5}b", PatternKind::Regex);
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
    TypeParam automaton("(cat|dog|elephant)s?", PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    EXPECT_TRUE(automaton.isPartialMatch("ca"));
    EXPECT_TRUE(automaton.isPartialMatch("do"));
    EXPECT_TRUE(automaton.isPartialMatch("ele"));

    // Partial match with soft mode (is_soft = true)
    EXPECT_FALSE(automaton.isPartialMatch("cat", true));

    // Partial match with hard mode (is_soft = false)
    EXPECT_TRUE(automaton.isPartialMatch("cat"));
    EXPECT_TRUE(automaton.isPartialMatch("cat", false));

    const std::string padded = std::string(64, 'e') + "ele";
    EXPECT_TRUE(automaton.isPartialMatch(padded));

    EXPECT_FALSE(automaton.isMatch("ca"));
    EXPECT_TRUE(automaton.isMatch("cat"));
    EXPECT_TRUE(automaton.isMatch("dogs"));
  }

  // Partial match under anchors
  // {
  //   TypeParam automaton("^hello world$", PatternKind::Regex));
  //   ASSERT_TRUE(automaton.ok());
  //   EXPECT_TRUE(automaton.isPartialMatch("hello"));
  //   EXPECT_TRUE(automaton.isPartialMatch("hello wor"));
  //   EXPECT_FALSE(automaton.isMatch("hello"));
  //   EXPECT_TRUE(automaton.isMatch("hello world"));
  // }

  // Assertion prefix
  // {
  //   TypeParam automaton("foo(?=bar)", PatternKind::Regex);
  //   ASSERT_TRUE(automaton.ok());
  //   EXPECT_TRUE(automaton.isPartialMatch("foo"));
  //   EXPECT_TRUE(automaton.isPartialMatch("foob"));
  //   EXPECT_FALSE(automaton.isMatch("foo"));
  //   EXPECT_TRUE(automaton.isMatch("foobar"));
  // }
}

TYPED_TEST_P(CommonAutomataMatchTest, LiteralMatch) {
  std::vector<std::string> patterns = {".+", "ab?", "(ab)+"};
  TypeParam automaton(patterns, PatternKind::Literal);
  ASSERT_TRUE(automaton.ok());

  EXPECT_TRUE(automaton.isMatch(".+"));
  EXPECT_TRUE(automaton.isMatch("ab?"));
  EXPECT_TRUE(automaton.isMatch("(ab)+"));
  EXPECT_FALSE(automaton.isMatch("c"));
  EXPECT_FALSE(automaton.isMatch("a"));
  EXPECT_FALSE(automaton.isMatch("ab"));

  EXPECT_TRUE(automaton.isPartialMatch("."));
  EXPECT_TRUE(automaton.isPartialMatch("ab"));
  EXPECT_TRUE(automaton.isPartialMatch("(ab"));
}

TYPED_TEST_P(CommonAutomataMatchTest, Search) {
  auto searchAndSort = [](TypeParam& automaton, const std::string& input, bool anchored) {
    std::vector<MatchInfo> matches;
    automaton.search(input, matches, anchored);

    // The order of pattern_index_ in the matches may be different between DFA and NFA, so we sort
    // them for consistent testing.
    std::sort(matches.begin(), matches.end(), [](const MatchInfo& a, const MatchInfo& b) {
      return a.pattern_index_ < b.pattern_index_;
    });

    return matches;
  };

  TypeParam automaton("hello(world)?", PatternKind::Regex);
  ASSERT_TRUE(automaton.ok());

  // Test search with anchored = true
  auto matches = searchAndSort(automaton, "hello", true);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 0);
  EXPECT_EQ(matches[0].length_, 5);
  matches = searchAndSort(automaton, "world hello", true);
  EXPECT_EQ(matches.size(), 0);

  // Test search with anchored = false
  matches = searchAndSort(automaton, "hello", false);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 0);
  EXPECT_EQ(matches[0].length_, 5);
  matches = searchAndSort(automaton, "world hello", false);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 6);
  EXPECT_EQ(matches[0].length_, 5);

  // Test search with multiple patterns
  {
    std::vector<std::string> patterns = {"hello(world)?", "world", "hello", ".+"};
    TypeParam automaton(patterns, PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    automaton.toDotSvg("1.svg");

    // With anchored = true
    std::vector<MatchInfo> matches;
    matches = searchAndSort(automaton, "hello", true);
    ASSERT_EQ(matches.size(), 3);
    EXPECT_EQ(matches[0].pattern_index_, 0);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 2);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 5);
    EXPECT_EQ(matches[2].pattern_index_, 3);
    EXPECT_EQ(matches[2].start_offset_, 0);
    EXPECT_EQ(matches[2].length_, 5);
    matches = searchAndSort(automaton, "worldhello", true);
    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(matches[0].pattern_index_, 1);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 3);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 10);
    matches = searchAndSort(automaton, "hi, world", true);
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].pattern_index_, 3);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 9);

    // With anchored = false
    matches = searchAndSort(automaton, "hello", false);
    ASSERT_EQ(matches.size(), 7);
    EXPECT_EQ(matches[0].pattern_index_, 0);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 2);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 5);
    EXPECT_EQ(matches[2].pattern_index_, 3);
    EXPECT_EQ(matches[2].start_offset_, 0);
    EXPECT_EQ(matches[2].length_, 5);
    EXPECT_EQ(matches[3].pattern_index_, 3);
    EXPECT_EQ(matches[3].start_offset_, 1);
    EXPECT_EQ(matches[3].length_, 4);
    EXPECT_EQ(matches[4].pattern_index_, 3);
    EXPECT_EQ(matches[4].start_offset_, 2);
    EXPECT_EQ(matches[4].length_, 3);
    EXPECT_EQ(matches[5].pattern_index_, 3);
    EXPECT_EQ(matches[5].start_offset_, 3);
    EXPECT_EQ(matches[5].length_, 2);
    EXPECT_EQ(matches[6].pattern_index_, 3);
    EXPECT_EQ(matches[6].start_offset_, 4);
    EXPECT_EQ(matches[6].length_, 1);
    matches = searchAndSort(automaton, "worldhello", false);
    ASSERT_EQ(matches.size(), 13);
    EXPECT_EQ(matches[0].pattern_index_, 0);
    EXPECT_EQ(matches[0].start_offset_, 5);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 1);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 5);
    EXPECT_EQ(matches[2].pattern_index_, 2);
    EXPECT_EQ(matches[2].start_offset_, 5);
    EXPECT_EQ(matches[2].length_, 5);
    EXPECT_EQ(matches[3].pattern_index_, 3);
    EXPECT_EQ(matches[3].start_offset_, 0);
    EXPECT_EQ(matches[3].length_, 10);
    EXPECT_EQ(matches[4].pattern_index_, 3);
    EXPECT_EQ(matches[4].start_offset_, 1);
    EXPECT_EQ(matches[4].length_, 9);
    EXPECT_EQ(matches[5].pattern_index_, 3);
    EXPECT_EQ(matches[5].start_offset_, 2);
    EXPECT_EQ(matches[5].length_, 8);
    EXPECT_EQ(matches[6].pattern_index_, 3);
    EXPECT_EQ(matches[6].start_offset_, 3);
    EXPECT_EQ(matches[6].length_, 7);
    EXPECT_EQ(matches[7].pattern_index_, 3);
    EXPECT_EQ(matches[7].start_offset_, 4);
    EXPECT_EQ(matches[7].length_, 6);
    EXPECT_EQ(matches[8].pattern_index_, 3);
    EXPECT_EQ(matches[8].start_offset_, 5);
    EXPECT_EQ(matches[8].length_, 5);
    EXPECT_EQ(matches[9].pattern_index_, 3);
    EXPECT_EQ(matches[9].start_offset_, 6);
    EXPECT_EQ(matches[9].length_, 4);
    EXPECT_EQ(matches[10].pattern_index_, 3);
    EXPECT_EQ(matches[10].start_offset_, 7);
    EXPECT_EQ(matches[10].length_, 3);
    EXPECT_EQ(matches[11].pattern_index_, 3);
    EXPECT_EQ(matches[11].start_offset_, 8);
    EXPECT_EQ(matches[11].length_, 2);
    EXPECT_EQ(matches[12].pattern_index_, 3);
    EXPECT_EQ(matches[12].start_offset_, 9);
    EXPECT_EQ(matches[12].length_, 1);
    matches = searchAndSort(automaton, "hi, world", false);
    ASSERT_EQ(matches.size(), 10);
    EXPECT_EQ(matches[0].pattern_index_, 1);
    EXPECT_EQ(matches[0].start_offset_, 4);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 3);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 9);
    EXPECT_EQ(matches[2].pattern_index_, 3);
    EXPECT_EQ(matches[2].start_offset_, 1);
    EXPECT_EQ(matches[2].length_, 8);
    EXPECT_EQ(matches[3].pattern_index_, 3);
    EXPECT_EQ(matches[3].start_offset_, 2);
    EXPECT_EQ(matches[3].length_, 7);
    EXPECT_EQ(matches[4].pattern_index_, 3);
    EXPECT_EQ(matches[4].start_offset_, 3);
    EXPECT_EQ(matches[4].length_, 6);
    EXPECT_EQ(matches[5].pattern_index_, 3);
    EXPECT_EQ(matches[5].start_offset_, 4);
    EXPECT_EQ(matches[5].length_, 5);
    EXPECT_EQ(matches[6].pattern_index_, 3);
    EXPECT_EQ(matches[6].start_offset_, 5);
    EXPECT_EQ(matches[6].length_, 4);
    EXPECT_EQ(matches[7].pattern_index_, 3);
    EXPECT_EQ(matches[7].start_offset_, 6);
    EXPECT_EQ(matches[7].length_, 3);
    EXPECT_EQ(matches[8].pattern_index_, 3);
    EXPECT_EQ(matches[8].start_offset_, 7);
    EXPECT_EQ(matches[8].length_, 2);
    EXPECT_EQ(matches[9].pattern_index_, 3);
    EXPECT_EQ(matches[9].start_offset_, 8);
    EXPECT_EQ(matches[9].length_, 1);
  }
}

TYPED_TEST_P(CommonAutomataMatchTest, SearchLongest) {
  // Test the longest match with single pattern
  TypeParam automaton("hello(world)?", PatternKind::Regex);
  ASSERT_TRUE(automaton.ok());
  MatchInfo match;
  EXPECT_TRUE(automaton.searchLongest("hello", 5, match));
  EXPECT_EQ(match.pattern_index_, 0);
  EXPECT_EQ(match.start_offset_, 0);
  EXPECT_EQ(match.length_, 5);
  EXPECT_FALSE(automaton.searchLongest("worldhello", 10, match));

  // Test the longest match with multiple patterns
  {
    std::vector<std::string> patterns = {"hello(world)?", "world", "hello", ".+"};
    TypeParam automaton(patterns, PatternKind::Regex);
    ASSERT_TRUE(automaton.ok());
    MatchInfo match;
    EXPECT_TRUE(automaton.searchLongest("hello", 5, match));
    EXPECT_EQ(match.pattern_index_, 0);
    EXPECT_EQ(match.start_offset_, 0);
    EXPECT_EQ(match.length_, 5);
    EXPECT_TRUE(automaton.searchLongest("worldhello", 10, match));
    EXPECT_EQ(match.pattern_index_, 3);
    EXPECT_EQ(match.start_offset_, 0);
    EXPECT_EQ(match.length_, 10);
    EXPECT_TRUE(automaton.searchLongest("hi, world", 9, match));
    EXPECT_EQ(match.pattern_index_, 3);
    EXPECT_EQ(match.start_offset_, 0);
    EXPECT_EQ(match.length_, 9);
  }
}

REGISTER_TYPED_TEST_SUITE_P(CommonAutomataMatchTest, Constructor, Serialization, IsMatchBasic,
                            IsMatchWithQuantifiers, IsMatchWithAlternation, LongestPath,
                            IsPartialMatch, LiteralMatch, Search, SearchLongest);

} // namespace Sregex::Automata