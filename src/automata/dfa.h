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

#include <array>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "automaton.h"
#include "transition.h"

namespace Sregex::Automata {
class Dfa : public Automaton<Dfa> {
  friend class Automaton<Dfa>;
  friend class Nfa;

public:
  Dfa() = default;
  Dfa(const std::string& pattern);
  Dfa(const std::span<const std::string>& patterns);
  Dfa(const Dfa& other) = delete;
  Dfa& operator=(const Dfa& other) = delete;
  Dfa(Dfa&& other) = default;
  Dfa& operator=(Dfa&& other) = default;

protected:
  // Implementation of the isMatch interface
  bool isMatchImpl(const char* input, size_t length) const;

  // Implementation of the isPartialMatch interface
  bool isPartialMatchImpl(const char* input, size_t length) const;

  // Implementation of the longestPath interface
  size_t longestPathImpl() const;

  // Implementation of the toDot interface
  std::string toDotImpl() const;

private:
  // Normalize the DFA by sorting the transitions of each state.
  void normalize();

  // Use partition refinement(Moore) to minimize the DFA.
  void minimize();
  void buildTransitionsTables();

private:
  struct State {
    bool is_accept_state_ = false;
    std::vector<Transition> transitions_;

    // 128 ASCII characters, each entry is a u16 representing the target state ID
    using TableStateID = uint32_t;
    std::array<TableStateID, 128> transition_table_;

    State() { transition_table_.fill(std::numeric_limits<TableStateID>::max()); }

    void buildTransitionsTable();
  };

private:
  std::vector<State> states_;
  StateId start_state_;
};
} // namespace Sregex::Automata