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
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

#include <assert.h>

#include "dfa.h"
#include "nfa_builder.h"

namespace Sregex::Automata {
Nfa::Nfa(const std::string& pattern) {
  *this = NfaBuilder::withPattern(pattern);
  if (ok()) {
    buildEpsilonClosureCache();
  }
}

Nfa::Nfa(const std::span<const std::string>& patterns) {
  *this = NfaBuilder::withPatterns(patterns);
  if (ok()) {
    buildEpsilonClosureCache();
  }
}

Dfa Nfa::toDfa() const {
  struct VecHash {
    std::size_t operator()(const std::vector<StateId>& v) const noexcept {
      std::size_t h = 0;
      for (StateId x : v) {
        h ^= std::hash<StateId>{}(x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
      }
      return h;
    }
  };

  std::vector<StateId> start_ids = epsilonClosure(start_state_);
  std::unordered_set<StateId> accept_ids;
  if (std::any_of(start_ids.begin(), start_ids.end(),
                  [this](StateId id) { return id == accept_state_; })) {
    accept_ids.insert(0);
  }

  std::vector<std::vector<StateId>> dfa_states(1, start_ids);
  std::vector<std::vector<Transition>> dfa_transitions(1);
  std::unordered_map<std::vector<StateId>, StateId, VecHash> dfa_state_ids;

  std::queue<StateId> queue;
  queue.push(0);
  while (!queue.empty()) {
    StateId dfa_state_id = queue.front();
    queue.pop();

    // Collect all transitions from the NFA states in the current DFA state
    std::vector<std::pair<Transition, std::vector<StateId>>> transitions;
    for (StateId state : dfa_states[dfa_state_id]) {
      for (const auto& t : states_[state].transitions_) {
        std::vector<StateId> targets = epsilonClosure(t.target_);
        transitions.emplace_back(t, targets);
      }
    }
    if (transitions.empty()) {
      continue;
    }

    // Use the scanline algorithm to split overlapping transitions into non-overlapping intervals
    std::vector<std::pair<Transition, std::vector<StateId>>> split_transitions =
        splitTransitionsWithScanline(transitions);

    // Collect the transitions for the current DFA state
    std::vector<Transition> current_transitions;
    for (const auto& [range, targets] : split_transitions) {
      // Map the split transition to a unique DFA state
      StateId state_id;
      auto iter = dfa_state_ids.find(targets);
      if (iter != dfa_state_ids.end()) {
        state_id = iter->second;
      } else {
        state_id = dfa_states.size();

        // Check if the new DFA state is an accept state
        if (std::any_of(targets.begin(), targets.end(),
                        [this](StateId id) { return id == accept_state_; })) {
          accept_ids.insert(state_id);
        }

        // Add the new DFA state to the list and the mapping
        dfa_states.push_back(targets);
        dfa_transitions.emplace_back();

        dfa_state_ids[targets] = state_id;

        // Add the new DFA state to the queue for processing
        queue.push(state_id);
      }

      current_transitions.push_back({range.start_, range.end_, state_id});
    }

    // Store the collected transitions for the current DFA state
    dfa_transitions[dfa_state_id] = std::move(current_transitions);
  }

  assert(dfa_states.size() == dfa_transitions.size());

  return Dfa(dfa_transitions, accept_ids, longest_path_);
}

bool Nfa::isMatchImpl(const char* input, size_t length) const {
  return dfs(start_state_, input, length);
}

bool Nfa::isPartialMatchImpl(const char* input, size_t length, bool is_soft) const {
  size_t match_length = std::min(longest_path_, length);
  const char* match_start = input + (length - match_length);

  const size_t memo_width = length + 1;
  std::vector<int8_t> memo(states_.size() * memo_width, -1);
  for (size_t offset = 0; offset < match_length; ++offset) {
    if (dfsPartial(start_state_, match_start + offset, match_length - offset, memo, memo_width,
                   is_soft)) {
      return true;
    }
  }

  return false;
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

bool Nfa::dfsPartial(StateId state, const char* input, size_t length, std::vector<int8_t>& memo,
                     size_t memo_width, bool is_soft) const {
  const size_t memo_index = state * memo_width + length;
  if (memo[memo_index] != -1) {
    return memo[memo_index] == 1;
  }

  if (epsilon_closure_cache_[state].has_accept_state_) {
    const auto& closure_cache = epsilon_closure_cache_[state].closure_;
    bool has_continuation =
        std::any_of(closure_cache.begin(), closure_cache.end(), [this](StateId sid) {
          return !states_[sid].transitions_.empty() || !states_[sid].epsilons_.empty();
        });

    // A full match already exists for the current prefix. Soft mode rejects it as a partial match,
    // while hard mode only reports it as partial when the accept state can still continue.
    if (!has_continuation) {
      memo[memo_index] = 0;
      return false;
    }

    memo[memo_index] = is_soft ? 0 : 1;
    return !is_soft;
  }

  if (length == 0) {
    memo[memo_index] = 1;
    return true;
  }

  const std::vector<StateId>& closure_cache = epsilon_closure_cache_[state].closure_;
  for (StateId closure_state : closure_cache) {
    for (const auto& transition : states_[closure_state].transitions_) {
      if (transition.matches(input[0])) {
        if (dfsPartial(transition.target_, input + 1, length - 1, memo, memo_width, is_soft)) {
          memo[memo_index] = 1;
          return true;
        }
      }
    }
  }

  memo[memo_index] = 0;
  return false;
}

std::vector<std::pair<Transition, std::vector<StateId>>> Nfa::splitTransitionsWithScanline(
    const std::vector<std::pair<Transition, std::vector<StateId>>>& transitions) const {
  // Collect all unique points (start and end+1)
  std::vector<uint32_t> points;
  for (const auto& [transition, _] : transitions) {
    points.push_back(transition.start_);
    points.push_back(transition.end_ + 1);
  }
  std::sort(points.begin(), points.end());
  points.erase(std::unique(points.begin(), points.end()), points.end());

  // For each interval between points, find the target set
  std::vector<std::pair<Transition, std::vector<StateId>>> interval_targets;
  for (size_t i = 0; i < points.size() - 1; ++i) {
    uint32_t interval_start = points[i];
    uint32_t interval_end = points[i + 1] - 1;

    // Use std::set to maintain order and uniqueness of target states
    std::set<StateId> target_set;
    for (const auto& [range, targets] : transitions) {
      if (range.start_ <= interval_end && range.end_ >= interval_start) {
        target_set.insert(targets.begin(), targets.end());
      }
    }

    // Collect the target states for this interval if there are any
    if (!target_set.empty()) {
      interval_targets.emplace_back(Transition{static_cast<unsigned char>(interval_start),
                                               static_cast<unsigned char>(interval_end), 0},
                                    std::vector<StateId>(target_set.begin(), target_set.end()));
    }
  }

  // Merge adjacent intervals with the same target set
  std::vector<std::pair<Transition, std::vector<StateId>>> merged_intervals;
  for (const auto& [range, targets] : interval_targets) {
    if (!merged_intervals.empty()) {
      auto& last_range = merged_intervals.back().first;
      auto& last_targets = merged_intervals.back().second;
      if (last_range.end_ + 1 == range.start_ && last_targets == targets) {
        last_range.end_ = range.end_;
      }
    }
    merged_intervals.push_back({range, targets});
  }

  return merged_intervals;
}
} // namespace Sregex::Automata