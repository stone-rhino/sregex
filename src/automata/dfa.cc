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
#include <numeric>
#include <queue>
#include <stack>

#include "nfa.h"

namespace Sregex::Automata {
Dfa::Dfa(const std::string& pattern) {
  *this = Nfa(pattern).toDfa();
  normalize();
  minimize();
  reorderStatesForLocality();
  buildTransitionsTables();
}

Dfa::Dfa(const std::span<const std::string>& patterns) {
  Nfa nfa(patterns);
  *this = nfa.toDfa();
  normalize();
  minimize();
  reorderStatesForLocality();
  buildTransitionsTables();
}

bool Dfa::isMatchImpl(const char* input, size_t length) const {
  StateId current_state = start_state_;
  for (size_t offset = 0; offset < length; ++offset) {
    const char* current_input = input + offset;
    size_t remaining_length = length - offset;
    for (size_t i = 0; i < remaining_length; ++i) {
      std::optional<StateId> next = nextState(current_state, current_input[i]);
      if (next.has_value()) {
        current_state = next.value();
        if (states_[current_state].is_accept_state_) {
          return true;
        }
      } else {
        break;
      }
    }
    current_state = start_state_;
  }

  return false;
}

bool Dfa::isPartialMatchImpl(const char* input, size_t length) const { throw "Not implemented!"; }

size_t Dfa::longestPathImpl() const { throw "Not implemented!"; }

std::string Dfa::toDotImpl() const {
  std::string dot = "digraph DFA {\n";

  dot.append("  labelloc=\"t\";\n");
  dot.append("  labeljust=\"l\";\n");
  std::string selected =
      bfs_metrics_.total_abs_delta_ < dfs_metrics_.total_abs_delta_ ? "BFS" : "DFS";
  dot.append(std::format("  // reorder: selected={}, bfs_score={}, dfs_score={}\n", selected,
                         bfs_metrics_.total_abs_delta_, dfs_metrics_.total_abs_delta_));
  dot.append(std::format(
      "  debug_info [shape=note, fontsize=10, label=\""
      "reorder: {}\\l"
      "bfs_score: {}\\l"
      "dfs_score: {}\\l\\l"
      "BFS edge_count: {}\\l"
      "BFS avg_abs_delta: {:.3}\\l"
      "BFS p50/p90/max: {}/{}/{}\\l"
      "BFS adjacent/near2/near4/long8: {:.3}/{:.3}/{:.3}/{:.3}\\l"
      "BFS start_edge_count/start_avg: {}/{:.3}\\l\\l"
      "DFS edge_count: {}\\l"
      "DFS avg_abs_delta: {:.3}\\l"
      "DFS p50/p90/max: {}/{}/{}\\l"
      "DFS adjacent/near2/near4/long8: {:.3}/{:.3}/{:.3}/{:.3}\\l"
      "DFS start_edge_count/start_avg: {}/{:.3}\\l\"];\n",
      selected, bfs_metrics_.total_abs_delta_, dfs_metrics_.total_abs_delta_,
      bfs_metrics_.edge_count_, bfs_metrics_.avg_abs_delta_, bfs_metrics_.p50_abs_delta_,
      bfs_metrics_.p90_abs_delta_, bfs_metrics_.max_abs_delta_, bfs_metrics_.adjacent_ratio_,
      bfs_metrics_.near2_ratio_, bfs_metrics_.near4_ratio_, bfs_metrics_.long8_ratio_,
      bfs_metrics_.start_edge_count_, bfs_metrics_.start_avg_abs_delta_, dfs_metrics_.edge_count_,
      dfs_metrics_.avg_abs_delta_, dfs_metrics_.p50_abs_delta_, dfs_metrics_.p90_abs_delta_,
      dfs_metrics_.max_abs_delta_, dfs_metrics_.adjacent_ratio_, dfs_metrics_.near2_ratio_,
      dfs_metrics_.near4_ratio_, dfs_metrics_.long8_ratio_, dfs_metrics_.start_edge_count_,
      dfs_metrics_.start_avg_abs_delta_));
  dot.append("  debug_info -> start [style=dashed, arrowhead=none, color=gray60];\n");

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
              [](const Transition& a, const Transition& b) {
                if (a.start_ != b.start_) {
                  return a.start_ < b.start_;
                }
                return a.end_ < b.end_;
              });
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
    using Signature = std::vector<std::tuple<PartitionID, RangeStart, RangeEnd, PartitionID>>;
    using States = std::vector<StateId>;

    std::map<Signature, States> signatures;
    for (size_t state_id = 0; state_id < states_.size(); ++state_id) {
      const auto& state = states_[state_id];
      // Calculate the signature for the current state.
      // The signature consists of the ranges of the transitions and the partition IDs of their
      // targets.
      Signature signature;
      signature.reserve(state.transitions_.size());
      for (const auto& transition : state.transitions_) {
        signature.emplace_back(partition_ids[state_id], transition.start_, transition.end_,
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

void Dfa::reorderStatesForLocality() {
  if (states_.size() <= 1) {
    return;
  }

  auto bfs_map = buildReorderedMapBFS();
  auto dfs_map = buildReorderedMapDFS();
  computeReorderMetrics(bfs_map, bfs_metrics_);
  computeReorderMetrics(dfs_map, dfs_metrics_);
  if (bfs_metrics_.total_abs_delta_ < dfs_metrics_.total_abs_delta_) {
    applyReorderedMap(bfs_map);
  } else {
    applyReorderedMap(dfs_map);
  }
}

std::vector<StateId> Dfa::buildReorderedMapBFS() const {
  std::vector<std::optional<StateId>> old_to_new(states_.size());
  std::queue<StateId> queue;
  StateId next_id = 0;

  old_to_new[start_state_] = next_id;
  ++next_id;
  queue.push(start_state_);

  while (!queue.empty()) {
    StateId sid = queue.front();
    queue.pop();

    std::vector<StateId> targets;
    for (const auto& transition : states_[sid].transitions_) {
      targets.push_back(transition.target_);
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

    for (StateId target : targets) {
      if (old_to_new[target] == std::nullopt) {
        old_to_new[target] = next_id;
        ++next_id;
        queue.push(target);
      }
    }
  }

  for (auto& slot : old_to_new) {
    if (!slot.has_value()) {
      slot = next_id;
      ++next_id;
    }
  }

  std::vector<StateId> result;
  for (const auto& slot : old_to_new) {
    result.push_back(slot.value());
  }
  return result;
}

std::vector<StateId> Dfa::buildReorderedMapDFS() const {
  std::vector<std::optional<StateId>> old_to_new(states_.size());
  std::stack<StateId> stack;
  StateId next_id = 0;

  stack.push(start_state_);

  while (!stack.empty()) {
    StateId sid = stack.top();
    stack.pop();

    if (old_to_new[sid] == std::nullopt) {
      continue;
    }

    old_to_new[sid] = next_id;
    ++next_id;

    std::vector<StateId> targets;
    for (const auto& transition : states_[sid].transitions_) {
      targets.push_back(transition.target_);
    }
    std::sort(targets.begin(), targets.end());
    targets.erase(std::unique(targets.begin(), targets.end()), targets.end());

    // Reverse push so the smallest target is visited first on pop
    for (auto iter = targets.rbegin(); iter != targets.rend(); ++iter) {
      StateId target = *iter;
      if (old_to_new[target] == std::nullopt) {
        stack.push(target);
      }
    }
  }

  for (auto& slot : old_to_new) {
    if (!slot.has_value()) {
      slot = next_id;
      ++next_id;
    }
  }

  std::vector<StateId> result;
  for (const auto& slot : old_to_new) {
    result.push_back(slot.value());
  }
  return result;
}

void Dfa::computeReorderMetrics(const std::vector<StateId>& old_to_new,
                                ReorderMetrics& metrics) const {
  std::vector<size_t> deltas;
  std::vector<size_t> start_deltas;

  for (size_t state_id = 0; state_id < states_.size(); ++state_id) {
    const auto& state = states_[state_id];
    StateId src = old_to_new[state_id];
    for (const auto& t : state.transitions_) {
      StateId dst = old_to_new[t.target_];
      size_t delta = (src > dst) ? (src - dst) : (dst - src);
      deltas.push_back(delta);
      if (state_id == start_state_) {
        start_deltas.push_back(delta);
      }
    }
  }

  if (deltas.empty()) {
    return;
  }

  metrics.edge_count_ = deltas.size();
  metrics.total_abs_delta_ = std::accumulate(deltas.begin(), deltas.end(), size_t(0));
  metrics.avg_abs_delta_ = static_cast<double>(metrics.total_abs_delta_) / metrics.edge_count_;

  size_t adjacent_count = 0;
  size_t near2_count = 0;
  size_t near4_count = 0;
  size_t long8_count = 0;
  for (size_t delta : deltas) {
    if (delta == 1) {
      ++adjacent_count;
    }
    if (delta <= 2) {
      ++near2_count;
    }
    if (delta <= 4) {
      ++near4_count;
    }
    if (delta >= 8) {
      ++long8_count;
    }
  }
  metrics.adjacent_ratio_ = static_cast<double>(adjacent_count) / metrics.edge_count_;
  metrics.near2_ratio_ = static_cast<double>(near2_count) / metrics.edge_count_;
  metrics.near4_ratio_ = static_cast<double>(near4_count) / metrics.edge_count_;
  metrics.long8_ratio_ = static_cast<double>(long8_count) / metrics.edge_count_;

  std::sort(deltas.begin(), deltas.end());
  metrics.p50_abs_delta_ = deltas[metrics.edge_count_ / 2];
  metrics.p90_abs_delta_ = deltas[(metrics.edge_count_ - 1) * 9 / 10];
  metrics.max_abs_delta_ = deltas.back();

  metrics.start_edge_count_ = start_deltas.size();
  metrics.start_avg_abs_delta_ = metrics.start_edge_count_ == 0
                                     ? 0.0
                                     : static_cast<double>(std::accumulate(
                                           start_deltas.begin(), start_deltas.end(), size_t(0))) /
                                           metrics.start_edge_count_;
}

void Dfa::applyReorderedMap(const std::vector<StateId>& old_to_new) {
  std::vector<State> reordered(states_.size());

  for (StateId old_id = 0; old_id < states_.size(); ++old_id) {
    StateId new_id = old_to_new[old_id];
    State& old_state = states_[old_id];
    reordered[new_id] = std::move(old_state);
  }

  start_state_ = old_to_new[start_state_];
  states_ = std::move(reordered);
}

void Dfa::buildTransitionsTables() {
  for (auto& state : states_) {
    state.buildTransitionsTable();
  }
}

std::optional<StateId> Dfa::nextState(StateId current, char input) const {
  const State& state = states_[current];

  if (input < 128) {
    // Lookup the transition table
    if (state.transition_table_[input] != std::numeric_limits<State::TableStateID>::max()) {
      return static_cast<StateId>(state.transition_table_[input]);
    }
  } else {
    for (const auto& t : state.transitions_) {
      if (t.matches(input)) {
        return t.target_;
      }
    }
  }

  return std::nullopt;
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