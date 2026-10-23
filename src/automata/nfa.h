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

#include <cstdint>
#include <span>
#include <vector>

#include "automaton.h"
#include "src/parser/hir_kind.h"
#include "transition.h"

namespace Sregex::Automata {
class Dfa;

/**
 * A non-deterministic finite automaton (NFA) for pattern matching
 */
class Nfa : public Automaton<Nfa> {
  friend class NfaBuilder;
  friend class Automaton<Nfa>;

public:
  Nfa() = default;
  Nfa(const std::string& pattern);
  Nfa(const std::span<const std::string>& patterns);
  Nfa(const Nfa& other) = delete;
  Nfa& operator=(const Nfa& other) = delete;
  Nfa(Nfa&& other) = default;
  Nfa& operator=(Nfa&& other) = default;

public:
  /**
   * Converts the NFA to an equivalent DFA
   *
   * @return the resulting DFA
   */
  Dfa toDfa() const;

protected:
  // Implementation of the isMatch interface
  bool isMatchImpl(const char* input, size_t length) const;

  // Implementation of the isPartialMatch interface
  bool isPartialMatchImpl(const char* input, size_t length) const;

  // Implementation of the longestPath interface
  size_t longestPathImpl() const { return longest_path_; }

  // Implementation of the toDot interface
  std::string toDotImpl() const;

private:
  // Computes the epsilon closure of a given state
  std::vector<StateId> epsilonClosure(StateId state) const;

  // Builds the epsilon closure cache for all states
  void buildEpsilonClosureCache();

  // Performs a depth-first search to check if the NFA accepts the input string starting from the
  // given state and position
  bool dfs(StateId state, const char* input, size_t length) const;

  // Performs a depth-first search to check if the NFA partially accepts the input string starting
  // from the given state and position
  bool dfsPartial(StateId state, const char* input, size_t length, std::vector<int8_t>& memo,
                  size_t memo_width) const;

  // Computes the length of the longest path in the NFA starting from the given state
  int64_t longestPathFrom(StateId state, std::vector<bool>& visited) const;

  // Splits overlapping transitions into non-overlapping intervals using the scanline algorithm
  std::vector<std::pair<Transition, std::vector<StateId>>> splitTransitionsWithScanline(
      const std::vector<std::pair<Transition, std::vector<StateId>>>& transitions) const;

private:
  struct State {
    std::vector<Transition> transitions_;
    std::vector<StateId> epsilons_;
    void addTransition(unsigned char c, StateId target) {
      transitions_.push_back(
          {static_cast<unsigned char>(c), static_cast<unsigned char>(c), target});
    }
    void addTransition(unsigned char start, unsigned char end, StateId target) {
      transitions_.push_back({start, end, target});
    }
    void addEpsilon(StateId target) { epsilons_.emplace_back(target); }
  };

  struct ClosureCache {
    std::vector<StateId> closure_;
    bool has_accept_state_ = false;
  };

private:
  std::vector<State> states_;
  std::vector<ClosureCache> epsilon_closure_cache_;
  StateId start_state_;
  StateId accept_state_;
  size_t longest_path_;
};
} // namespace Sregex::Automata