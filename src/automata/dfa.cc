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
#include "dfa.h"

#include <algorithm>
#include <format>
#include <map>

#include "nfa.h"

namespace Sregex::Automata {
Dfa::Dfa(const std::string& pattern) {
  *this = Nfa(pattern).toDfa();
  normalize();
  minimize();
  buildTransitionsTables();
}

Dfa::Dfa(const std::span<const std::string>& patterns) {
  Nfa nfa(patterns);
  *this = nfa.toDfa();
  normalize();
  minimize();
  buildTransitionsTables();
}

bool Dfa::isMatchImpl(const char* input, size_t length) const { throw "Not implemented!"; }

bool Dfa::isPartialMatchImpl(const char* input, size_t length) const { throw "Not implemented!"; }

size_t Dfa::longestPathImpl() const { throw "Not implemented!"; }

std::string Dfa::toDotImpl() const {
  std::string dot = "digraph DFA {\n";
  dot.append("  rankdir=LR;\n");

  // Add an invisible start node that points to the actual start state
  dot.append("  start [shape=point];\n");
  dot.append(std::format("  start -> {};\n", start_state_));

  // Add the states and transitions
  dot.append("  node [shape=circle];\n");
  for (size_t i = 0; i < states_.size(); ++i) {
    const State& state = states_[i];

    // Add the state with a double circle if it's an accept state
    if (state.is_accept_state_) {
      dot.append(std::format("  {} [shape=doublecircle];\n", i));
    } else {
      dot.append(std::format("  {};\n", i));
    }

    // Group transition table edges by target and merge contiguous ASCII runs.
    // This keeps the graph semantics but dramatically reduces DOT size.
    std::map<State::TableStateID, std::vector<std::pair<uint8_t, uint8_t>>> grouped_runs;
    size_t idx = 0;
    while (idx < state.transition_table_.size()) {
      State::TableStateID target = state.transition_table_[idx];
      if (target == std::numeric_limits<State::TableStateID>::max()) {
        ++idx;
        continue;
      }

      uint8_t run_start = idx;
      uint8_t run_end = run_start;
      ++idx;
      while (idx < state.transition_table_.size() && state.transition_table_[idx] == target) {
        run_end = idx;
        ++idx;
      }

      grouped_runs[target].emplace_back(run_start, run_end);
    }

    for (const auto& [target, runs] : grouped_runs) {
      std::string label;
      for (const auto& [start, end] : runs) {
        if (!label.empty()) {
          label.append(", ");
        }
        if (start == end) {
          label.append(std::format("{}", scalarLabelToken(start)));
        } else {
          label.append(std::format("{}-{}", scalarLabelToken(start), scalarLabelToken(end)));
        }
      }
      dot.append(std::format("  {} -> {} [label=\"TT: [{}]\"];\n", i, target, label));
    }

    // Add the transitions for this state
    for (const auto& t : state.transitions_) {
      if (t.start_ == t.end_) {
        dot.append(
            std::format("  {} -> {} [label=\"{}\"];\n", i, t.target_, scalarLabelToken(t.start_)));
      } else {
        dot.append(std::format("  {} -> {} [label=\"{}-{}\"];\n", i, t.target_,
                               scalarLabelToken(t.start_), scalarLabelToken(t.end_)));
      }
    }
  }

  dot.append("}\n");

  return dot;
}

void Dfa::normalize() {
  for (auto& state : states_) {
    std::sort(state.transitions_.begin(), state.transitions_.end(),
              [](const Transition& a, const Transition& b) { return a.start_ < b.start_; });
  }
}

void Dfa::minimize() {
  // Initial partition: accepting and non-accepting states
  std::vector<std::vector<StateId>> partitions;
  std::vector<StateId> accept_partition;
  std::vector<StateId> non_accept_partition;
  for (StateId i = 0; i < states_.size(); ++i) {
    if (states_[i].is_accept_state_) {
      accept_partition.push_back(i);
    } else {
      non_accept_partition.push_back(i);
    }
  }
  if (!accept_partition.empty()) {
    partitions.push_back(std::move(accept_partition));
  }
  if (!non_accept_partition.empty()) {
    partitions.push_back(std::move(non_accept_partition));
  }

  // Only for beautifying dot output
  std::sort(partitions.begin(), partitions.end(),
            [](const std::vector<StateId>& a, const std::vector<StateId>& b) {
              return *std::min_element(a.begin(), a.end()) < *std::min_element(b.begin(), b.end());
            });

  using PartitionID = size_t;
  std::vector<PartitionID> partition_ids(states_.size(), 0);
  for (PartitionID pid = 0; pid < partitions.size(); ++pid) {
    for (StateId sid : partitions[pid]) {
      partition_ids[sid] = pid;
    }
  }

  // Loop until no more partitions(Moore) can be refined
  while (1) {
    using RangeStart = uint32_t;
    using RangeEnd = uint32_t;
    using Signature = std::vector<std::tuple<RangeStart, RangeEnd, PartitionID>>;
    using States = std::vector<StateId>;

    std::map<Signature, States> signatures;
    for (size_t state_id = 0; state_id < states_.size(); ++state_id) {
      const auto& state = states_[state_id];
      // Calculate the signature for the current state.
      // The signature consists of the labels (as ranges) of the transitions and the partition IDs
      // of their targets.
      Signature signature;
      signature.reserve(state.transitions_.size());
      for (const auto& transition : state.transitions_) {
        signature.emplace_back(transition.start_, transition.end_,
                               partition_ids[transition.target_]);
      }
      signatures[signature].push_back(state_id);
    }

    // If there are more signatures than partitions, it means that some states in the same partition
    // have different signatures and need to be split into smaller partitions
    if (signatures.size() > partitions.size()) {
      // Update the partitions with the new signatures
      partitions.clear();
      for (const auto& [signature, states] : signatures) {
        partitions.push_back(states);
      }

      // Only for beautifying dot output
      std::sort(partitions.begin(), partitions.end(),
                [](const std::vector<StateId>& a, const std::vector<StateId>& b) {
                  return *std::min_element(a.begin(), a.end()) <
                         *std::min_element(b.begin(), b.end());
                });

      for (PartitionID pid = 0; pid < partitions.size(); ++pid) {
        for (StateId sid : partitions[pid]) {
          partition_ids[sid] = pid;
        }
      }
    } else {
      break;
    }
  }

  // Create a new set of states based on the refined partitions
  std::vector<State> new_states;
  StateId new_start = 0;
  for (PartitionID pid = 0; pid < partitions.size(); ++pid) {
    const auto& part = partitions[pid];
    std::vector<Transition> new_transitions;
    StateId representative = part[0];
    const State& state = states_[representative];
    for (const auto& transition : state.transitions_) {
      StateId target_partition = partition_ids[transition.target_];
      new_transitions.push_back({transition.start_, transition.end_, target_partition});
    }

    State new_state;
    new_state.is_accept_state_ = state.is_accept_state_;
    new_state.transitions_ = std::move(new_transitions);
    new_states.push_back(std::move(new_state));
    if (start_state_ == representative) {
      new_start = new_states.size() - 1;
    }
  }

  // Update the DFA
  states_ = std::move(new_states);
  start_state_ = new_start;
}

void Dfa::buildTransitionsTables() {
  for (auto& state : states_) {
    state.buildTransitionsTable();
  }
}

void Dfa::State::buildTransitionsTable() {
  std::vector<Transition> new_transitions;
  new_transitions.reserve(transitions_.size());

  for (const auto& t : transitions_) {
    if (t.target_ < std::numeric_limits<TableStateID>::max()) {
      if (t.start_ < 128) {
        if (t.end_ == t.start_) {
          transition_table_[t.start_] = t.target_;
        } else {
          if (t.end_ < 128) {
            for (size_t i = t.start_; i <= t.end_; ++i) {
              transition_table_[i] = t.target_;
            }
          } else {
            for (size_t i = t.start_; i < 128; ++i) {
              transition_table_[i] = t.target_;
            }

            new_transitions.push_back({128, t.end_, t.target_});
          }
        }
      } else {
        new_transitions.push_back(t);
      }
    } else {
      new_transitions.push_back(t);
    }
  }

  transitions_ = std::move(new_transitions);
}
} // namespace Sregex::Automata