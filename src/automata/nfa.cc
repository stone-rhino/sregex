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
#include "nfa.h"

#include <algorithm>
#include <format>
#include <stack>

#include "nfa_builder.h"

namespace Sregex::Automata {
Nfa::Nfa(const std::string& pattern) {
  *this = NfaBuilder::withPattern(pattern);
  if (ok()) {
    buildEpsilonClosureCache();
    longest_path_ = longestPath();
  }
}

Nfa::Nfa(const std::span<const std::string>& patterns) {
  *this = NfaBuilder::withPatterns(patterns);
  if (ok()) {
    buildEpsilonClosureCache();
    longest_path_ = longestPath();
  }
}

bool Nfa::isMatchImpl(const char* input, size_t length) const {
  return dfs(start_state_, input, length);
}

bool Nfa::isPartialMatchImpl(const char* input, size_t length) const {
  size_t match_length = std::min(longest_path_, length);
  const char* match_start = input + (length - match_length);
  return dfsPartial(start_state_, match_start, match_length);
}

size_t Nfa::longestPathImpl() const {
  std::vector<bool> visited(states_.size(), false);
  const int64_t longest_path = longestPathFrom(start_state_, visited);
  return longest_path < 0 ? 0 : static_cast<size_t>(longest_path);
}

std::string scalarLabelToken(unsigned char c) {
  if (c == ' ') {
    return "SP";
  }

  if (c == '"' || c == '\\') {
    return std::format("0x{:02x}", c);
  }

  if (c >= 0x21 && c <= 0x7e) {
    return std::format("{}", static_cast<char>(c));
  }

  return std::format("0x{:02x}", c);
}

std::string Nfa::toDotImpl() const {
  std::string dot = "digraph NFA {\n";
  dot.append("  rankdir=LR;\n");

  // Add an invisible start node that points to the actual start state
  dot.append("  start [shape=point];\n");
  dot.append(std::format("  start -> {};\n", start_state_));

  // Add the states and transitions
  dot.append("  node [shape=circle];\n");
  for (size_t i = 0; i < states_.size(); ++i) {
    // Add the state with a double circle if it's an accept state
    if (i == accept_state_) {
      dot.append(std::format("  {} [shape=doublecircle];\n", i));
    } else {
      dot.append(std::format("  {};\n", i));
    }

    // Add the transitions for this state
    for (const auto& transition : states_[i].transitions_) {
      if (transition.start_ != transition.end_) {
        dot.append(std::format("  {} -> {} [label=\"{}-{}\"];\n", i, transition.target_,
                               scalarLabelToken(transition.start_),
                               scalarLabelToken(transition.end_)));
      } else {
        dot.append(std::format("  {} -> {} [label=\"{}\"];\n", i, transition.target_,
                               scalarLabelToken(transition.start_)));
      }
    }

    // Add the epsilon transitions for this state
    for (auto eid : states_[i].epsilons_) {
      dot.append(std::format("  {} -> {} [label=\"ε\"];\n", i, eid));
    }
  }

  dot.append("}\n");

  return dot;
}

std::vector<StateId> Nfa::epsilonClosure(StateId state) const {
  std::vector<StateId> closure;
  std::stack<StateId> stack;
  std::vector<bool> visited(states_.size(), false);
  visited[state] = true;

  stack.push(state);
  while (!stack.empty()) {
    StateId current = stack.top();
    stack.pop();
    closure.push_back(current);
    for (auto next : states_[current].epsilons_) {
      if (!visited[next]) {
        stack.push(next);
        visited[next] = true;
      }
    }
  }

  // Sort and remove duplicates from the closure
  std::sort(closure.begin(), closure.end());
  closure.erase(std::unique(closure.begin(), closure.end()), closure.end());

  return closure;
}

void Nfa::buildEpsilonClosureCache() {
  epsilon_closure_cache_.resize(states_.size());
  for (StateId i = 0; i < states_.size(); ++i) {
    std::vector<StateId> closure = epsilonClosure(i);

    // Remove states that have no outgoing transitions and are not the accept state
    closure.erase(std::remove_if(closure.begin(), closure.end(),
                                 [this](StateId s) {
                                   return states_[s].transitions_.empty() && s != accept_state_;
                                 }),
                  closure.end());

    bool has_accept_state = false;
    for (auto s : closure) {
      if (s == accept_state_) {
        has_accept_state = true;
        break;
      }
    }
    epsilon_closure_cache_[i].has_accept_state_ = has_accept_state;
    epsilon_closure_cache_[i].closure_ = std::move(closure);
  }
}

bool Nfa::dfs(StateId state, const char* input, size_t length) const {
  if (epsilon_closure_cache_[state].has_accept_state_) {
    return true;
  }

  const std::vector<StateId>& closure_cache = epsilon_closure_cache_[state].closure_;
  for (size_t i = 0; i < length; ++i) {
    for (StateId state : closure_cache) {
      for (const auto& transition : states_[state].transitions_) {
        if (transition.matches(input[i])) {
          if (dfs(transition.target_, input + i + 1, length - i - 1)) {
            return true;
          }
        }
      }
    }

    if (start_state_ != state) {
      break;
    }
  }

  return false;
}

bool Nfa::dfsPartial(StateId state, const char* input, size_t length) const {
  if (epsilon_closure_cache_[state].has_accept_state_) {
    return false;
  }

  if (length == 0) {
    return true;
  }

  const std::vector<StateId>& closure_cache = epsilon_closure_cache_[state].closure_;
  for (size_t i = 0; i < length; ++i) {
    for (StateId state : closure_cache) {
      for (const auto& transition : states_[state].transitions_) {
        if (transition.matches(input[i])) {
          if (dfsPartial(transition.target_, input + i + 1, length - i - 1)) {
            return true;
          }
        }
      }
    }

    if (start_state_ != state) {
      break;
    }
  }

  return false;
}

int64_t Nfa::longestPathFrom(StateId state, std::vector<bool>& visited) const {
  if (state == accept_state_) {
    return 0;
  }

  if (visited[state]) {
    return -1;
  }

  visited[state] = true;

  int64_t max_length = -1;

  for (StateId next : states_[state].epsilons_) {
    const int64_t sub_length = longestPathFrom(next, visited);
    if (sub_length >= 0) {
      max_length = std::max(max_length, sub_length);
    }
  }

  for (const auto& transition : states_[state].transitions_) {
    const int64_t sub_length = longestPathFrom(transition.target_, visited);
    if (sub_length >= 0) {
      max_length = std::max(max_length, sub_length + 1);
    }
  }

  visited[state] = false;
  return max_length;
}

} // namespace Sregex::Automata