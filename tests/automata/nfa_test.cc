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
TEST(NfaTest, Search) {
  Nfa nfa("hello(world)?", PatternKind::Regex);
  ASSERT_TRUE(nfa.ok());

  // Test search with anchored = true
  std::vector<MatchInfo> matches;
  nfa.search("hello", matches, true);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 0);
  EXPECT_EQ(matches[0].length_, 5);
  matches.clear();
  nfa.search("world hello", matches, true);
  EXPECT_EQ(matches.size(), 0);

  // Test search with anchored = false
  matches.clear();
  nfa.search("hello", matches, false);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 0);
  EXPECT_EQ(matches[0].length_, 5);
  matches.clear();
  nfa.search("world hello", matches, false);
  EXPECT_EQ(matches.size(), 1);
  EXPECT_EQ(matches[0].pattern_index_, 0);
  EXPECT_EQ(matches[0].start_offset_, 6);
  EXPECT_EQ(matches[0].length_, 5);

  // Test search with multiple patterns
  {
    std::vector<std::string> patterns = {"hello(world)?", "world", "hello", ".+"};
    Nfa nfa(patterns, PatternKind::Regex);
    ASSERT_TRUE(nfa.ok());
    nfa.toDotSvg("1.svg");

    // With anchored = true
    std::vector<MatchInfo> matches;
    nfa.search("hello", matches, true);
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
    matches.clear();
    nfa.search("worldhello", matches, true);
    ASSERT_EQ(matches.size(), 2);
    EXPECT_EQ(matches[0].pattern_index_, 1);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 3);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 10);
    matches.clear();
    nfa.search("hi, world", matches, true);
    ASSERT_EQ(matches.size(), 1);
    EXPECT_EQ(matches[0].pattern_index_, 3);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 9);

    // With anchored = false
    matches.clear();
    nfa.search("hello", matches, false);
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
    matches.clear();
    nfa.search("worldhello", matches, false);
    ASSERT_EQ(matches.size(), 13);
    EXPECT_EQ(matches[0].pattern_index_, 1);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 5);
    EXPECT_EQ(matches[1].pattern_index_, 3);
    EXPECT_EQ(matches[1].start_offset_, 0);
    EXPECT_EQ(matches[1].length_, 10);
    EXPECT_EQ(matches[2].pattern_index_, 3);
    EXPECT_EQ(matches[2].start_offset_, 1);
    EXPECT_EQ(matches[2].length_, 9);
    EXPECT_EQ(matches[3].pattern_index_, 3);
    EXPECT_EQ(matches[3].start_offset_, 2);
    EXPECT_EQ(matches[3].length_, 8);
    EXPECT_EQ(matches[4].pattern_index_, 3);
    EXPECT_EQ(matches[4].start_offset_, 3);
    EXPECT_EQ(matches[4].length_, 7);
    EXPECT_EQ(matches[5].pattern_index_, 3);
    EXPECT_EQ(matches[5].start_offset_, 4);
    EXPECT_EQ(matches[5].length_, 6);
    EXPECT_EQ(matches[6].pattern_index_, 0);
    EXPECT_EQ(matches[6].start_offset_, 5);
    EXPECT_EQ(matches[6].length_, 5);
    EXPECT_EQ(matches[7].pattern_index_, 2);
    EXPECT_EQ(matches[7].start_offset_, 5);
    EXPECT_EQ(matches[7].length_, 5);
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
    matches.clear();
    nfa.search("hi, world", matches, false);
    ASSERT_EQ(matches.size(), 10);
    EXPECT_EQ(matches[0].pattern_index_, 3);
    EXPECT_EQ(matches[0].start_offset_, 0);
    EXPECT_EQ(matches[0].length_, 9);
    EXPECT_EQ(matches[1].pattern_index_, 3);
    EXPECT_EQ(matches[1].start_offset_, 1);
    EXPECT_EQ(matches[1].length_, 8);
    EXPECT_EQ(matches[2].pattern_index_, 3);
    EXPECT_EQ(matches[2].start_offset_, 2);
    EXPECT_EQ(matches[2].length_, 7);
    EXPECT_EQ(matches[3].pattern_index_, 3);
    EXPECT_EQ(matches[3].start_offset_, 3);
    EXPECT_EQ(matches[3].length_, 6);
    EXPECT_EQ(matches[4].pattern_index_, 1);
    EXPECT_EQ(matches[4].start_offset_, 4);
    EXPECT_EQ(matches[4].length_, 5);
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
} // namespace Sregex::Automata