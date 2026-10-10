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

TEST(NfaTest, isMatch) {
  Nfa nfa(std::vector<std::string>{"(你*|好+|(hello)?|(hi){2,4})+世界"});
  EXPECT_TRUE(nfa.isMatch("世界"));
  EXPECT_TRUE(nfa.isMatch("你世界"));
  EXPECT_TRUE(nfa.isMatch("好好世界"));
  EXPECT_TRUE(nfa.isMatch("hello世界"));
  EXPECT_TRUE(nfa.isMatch("hihihi世界"));
  EXPECT_TRUE(nfa.isMatch("你好hellohihi世界"));
  EXPECT_TRUE(nfa.isMatch("aaa你世界aaa"));
  EXPECT_TRUE(nfa.isMatch("aaa好好世界aaa"));
  EXPECT_TRUE(nfa.isMatch("aaahello世界aaa"));
  EXPECT_TRUE(nfa.isMatch("aaahihihi世界aaa"));
  EXPECT_TRUE(nfa.isMatch("aaa你好hellohihi世界aaa"));
}

} // namespace Sregex::Automata