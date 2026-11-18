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

#include <boost/unordered/unordered_flat_map.hpp>

#include "nfa.h"
#include "src/sregex.h"

namespace Sregex::Automata {
Dfa::Dfa(const std::string& pattern, PatternKind kind) {
  if (pattern.empty()) {
    err_ = "Pattern is empty";
    return;
  }

  Nfa nfa = Nfa(pattern, kind);
  if (nfa.ok()) {
    *this = nfa.toDfa();
  } else {
    err_ = nfa.err();
  }
}

Dfa::Dfa(const std::span<const std::string>& patterns, PatternKind kind) {
  Nfa nfa = Nfa(patterns, kind);
  if (nfa.ok()) {
    *this = nfa.toDfa();
  } else {
    err_ = nfa.err();
  }
}

Dfa::Dfa(const std::span<const std::string>& patterns, const std::span<const PatternKind>& kinds) {
  Nfa nfa = Nfa(patterns, kinds);
  if (nfa.ok()) {
    *this = nfa.toDfa();
  } else {
    err_ = nfa.err();
  }
}

Dfa::Dfa(const std::vector<std::vector<Transition>>& transitions,
         std::vector<std::vector<PatternId>> pattern_ids, size_t longest_path) {
  // Create the DFA prepared states from the collected transitions by NFA
  prepared_states_.resize(transitions.size());
  for (size_t i = 0; i < transitions.size(); ++i) {
    bool is_accept_state = !pattern_ids[i].empty();
    if (is_accept_state) {
      if (transitions[i].empty()) {
        prepared_states_[i].state_type_ = StateType::AcceptTerminal;
      } else {
        prepared_states_[i].state_type_ = StateType::AcceptContinuable;
      }
    } else {
      prepared_states_[i].state_type_ = StateType::Normal;
    }
    prepared_states_[i].transitions_ = std::move(transitions[i]);
    prepared_states_[i].pattern_ids_ = std::move(pattern_ids[i]);
  }
  start_state_ = 0;
  longest_path_ = longest_path;

  normalize();
  minimize();
  reorderStatesForLocality();

  // Build the DFA states from the prepared states
  states_.reserve(prepared_states_.size());
  for (auto& prepared : prepared_states_) {
    states_.emplace_back(std::move(prepared));
  }

  // Release the memory held by prepared_states_ after building the DFA states
  prepared_states_.clear();
  std::vector<PreparedState> v;
  prepared_states_.swap(v);
}

bool Dfa::isMatchImpl(const char* input, size_t length, bool anchored) const {
  for (size_t offset = 0; offset < length; ++offset) {
    StateId current_state = start_state_;
    const char* current_input = input + offset;
    size_t remaining_length = length - offset;
    for (size_t i = 0; i < remaining_length; ++i) {
      std::optional<StateId> next = nextState(current_state, current_input[i]);
      if (next.has_value()) {
        current_state = next.value();
        if (states_[current_state].isAcceptState()) {
          return true;
        }
      } else {
        break;
      }
    }

    if (anchored) {
      break;
    }
  }

  return false;
}

bool Dfa::isPartialMatchImpl(const char* input, size_t length, bool is_soft) const {
  size_t match_length = std::min(longest_path_, length);
  if (match_length == 0) {
    return false;
  }
  const char* match_start = input + (length - match_length);

  enum class RunOutcome : uint8_t {
    NoMatch,
    AcceptTerminal,
    AcceptContinuable,
    FullSuffixWithoutAccept
  };
  std::vector<RunOutcome> next_outcomes(states_.size(), RunOutcome::NoMatch);
  std::vector<RunOutcome> current_outcomes(states_.size(), RunOutcome::NoMatch);

  // Scan the tail window once from right to left and propagate the run outcome for every DFA state
  // at each position
  for (int64_t pos = match_length - 1; pos >= 0; --pos) {
    const bool is_last_pos = (pos == match_length - 1);
    for (StateId sid = 0; sid < states_.size(); ++sid) {
      std::optional<StateId> next = nextState(sid, match_start[pos]);
      if (next.has_value()) {
        switch (states_[next.value()].state_type_) {
        case StateType::AcceptTerminal:
          current_outcomes[sid] = RunOutcome::AcceptTerminal;
          break;
        case StateType::AcceptContinuable:
          current_outcomes[sid] = RunOutcome::AcceptContinuable;
          break;
        default:
          current_outcomes[sid] =
              is_last_pos ? RunOutcome::FullSuffixWithoutAccept : next_outcomes[next.value()];
          break;
        }
      } else {
        current_outcomes[sid] = RunOutcome::NoMatch;
      }
    }

    const RunOutcome start_outcome = current_outcomes[start_state_];
    switch (start_outcome) {
    case RunOutcome::AcceptTerminal:
      return false;
    case RunOutcome::AcceptContinuable:
      return !is_soft;
    case RunOutcome::FullSuffixWithoutAccept:
      return true;
    default:
      std::swap(current_outcomes, next_outcomes);
      break;
    }
  }

  return false;
}

void Dfa::searchImpl(const char* input, size_t length, std::vector<MatchInfo>& matches,
                     bool anchored) const {
  struct PatternOffset {
    PatternId pattern_id_;
    size_t start_offset_;
    bool operator==(const PatternOffset& other) const {
      return pattern_id_ == other.pattern_id_ && start_offset_ == other.start_offset_;
    }
  };

  struct PatternOffsetHash {
    std::size_t operator()(const PatternOffset& po) const {
      return std::hash<PatternId>()(po.pattern_id_) << 32 ^ std::hash<size_t>()(po.start_offset_);
    }
  };

  using MatchIndex = size_t;
  boost::unordered_flat_map<PatternOffset, MatchIndex, PatternOffsetHash> matches_map;
  for (size_t offset = 0; offset < length; ++offset) {
    StateId current_state = start_state_;
    const char* current_input = input + offset;
    size_t remaining_length = length - offset;
    for (size_t i = 0; i < remaining_length; ++i) {
      std::optional<StateId> next = nextState(current_state, current_input[i]);
      if (next.has_value()) {
        current_state = next.value();
        if (states_[current_state].isAcceptState()) {
          const std::vector<PatternId>& pattern_ids = states_[current_state].pattern_ids_;
          for (PatternId pid : pattern_ids) {
            PatternOffset po{pid, offset};
            auto iter = matches_map.find(po);
            if (iter == matches_map.end()) {
              matches_map.insert({po, matches.size()});
              matches.emplace_back(pid, offset, i + 1);
            } else {
              matches[iter->second].length_ = i + 1;
            }
          }
        }
      } else {
        break;
      }
    }

    if (anchored) {
      break;
    }
  }
}

bool Dfa::searchLongestImpl(const char* input, size_t length, MatchInfo& matches) const {
  bool found_match = false;

  StateId current_state = start_state_;
  for (size_t i = 0; i < length; ++i) {
    std::optional<StateId> next = nextState(current_state, input[i]);
    if (next.has_value()) {
      current_state = next.value();
      if (states_[current_state].isAcceptState()) {
        found_match = true;
        const std::vector<PatternId>& pattern_ids = states_[current_state].pattern_ids_;
        assert(!pattern_ids.empty());
        matches.pattern_index_ = pattern_ids.front();
        matches.start_offset_ = 0;
        matches.length_ = i + 1;
      }
    } else {
      break;
    }
  }

  return found_match;
}

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

    std::string node_label = std::format("{}", i);
    if (state.isAcceptState()) {
      std::string ids;
      for (const auto& pid : state.pattern_ids_) {
        if (!ids.empty()) {
          ids += ", ";
        }
        ids += std::to_string(pid);
      }
      node_label += std::format("\\n{}", ids);
    }

    // Add the state with a double circle if it's an accept state
    if (state.isAcceptState()) {
      dot.append(std::format("  {} [shape=doublecircle, label=\"{}\"];\n", i, node_label));
    } else {
      dot.append(std::format("  {} [label=\"{}\"];\n", i, node_label));
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
      dot.append(std::format("  {} -> {} [label=\"{}\"];\n", i, target, label));
    }
  }

  dot.append("}\n");

  return dot;
}

void Dfa::normalize() {
  for (auto& state : prepared_states_) {
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
  // Collect states into groups based on their output pattern IDs
  std::map<std::vector<PatternId>, std::vector<StateId>> output_groups;
  for (StateId sid = 0; sid < prepared_states_.size(); ++sid) {
    output_groups[prepared_states_[sid].pattern_ids_].push_back(sid);
  }

  // Initial partition based on output pattern IDs
  using PartitionID = size_t;
  std::vector<PartitionID> partition_ids(prepared_states_.size(), 0);
  std::vector<std::vector<StateId>> partitions;
  for (auto& [pattern_ids, states] : output_groups) {
    partitions.push_back(std::move(states));
  }
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

    std::vector<States> new_partitions;
    bool refined = false;
    for (const auto& part : partitions) {
      std::map<Signature, States> signatures;
      for (StateId sid : part) {
        const auto& state = prepared_states_[sid];
        // Calculate the signature for the current state.
        // The signature consists of the labels (as ranges) of the transitions and the partition IDs
        // of their targets.
        Signature signature;
        signature.reserve(state.transitions_.size());
        for (const auto& transition : state.transitions_) {
          signature.emplace_back(transition.start_, transition.end_,
                                 partition_ids[transition.target_]);
        }
        signatures[signature].push_back(sid);
      }
      if (signatures.size() > 1) {
        refined = true;
      }
      for (auto& [_, states] : signatures) {
        new_partitions.push_back(std::move(states));
      }
    }

    if (!refined) {
      break;
    }

    partitions = std::move(new_partitions);
    for (PartitionID pid = 0; pid < partitions.size(); ++pid) {
      for (StateId sid : partitions[pid]) {
        partition_ids[sid] = pid;
      }
    }
  }

  // Create a new set of states based on the refined partitions
  std::vector<PreparedState> new_states;
  StateId new_start_state = 0;
  for (PartitionID pid = 0; pid < partitions.size(); ++pid) {
    const auto& part = partitions[pid];
    std::vector<Transition> new_transitions;
    StateId representative = part[0];
    const PreparedState& state = prepared_states_[representative];
    for (const auto& transition : state.transitions_) {
      StateId target_partition = partition_ids[transition.target_];
      new_transitions.push_back({transition.start_, transition.end_, target_partition});
    }

    PreparedState new_state;
    new_state.state_type_ = state.state_type_;
    new_state.transitions_ = std::move(new_transitions);
    new_state.pattern_ids_ = std::move(state.pattern_ids_);
    new_states.push_back(std::move(new_state));
    if (start_state_ == representative) {
      new_start_state = new_states.size() - 1;
    }
  }

  // Update the DFA
  prepared_states_ = std::move(new_states);
  start_state_ = new_start_state;
}

void Dfa::reorderStatesForLocality() {
  if (prepared_states_.size() <= 1) {
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
  std::vector<std::optional<StateId>> old_to_new(prepared_states_.size());
  std::queue<StateId> queue;
  StateId next_id = 0;

  old_to_new[start_state_] = next_id;
  ++next_id;
  queue.push(start_state_);

  while (!queue.empty()) {
    StateId sid = queue.front();
    queue.pop();

    std::vector<StateId> targets;
    for (const auto& transition : prepared_states_[sid].transitions_) {
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
  std::vector<std::optional<StateId>> old_to_new(prepared_states_.size());
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
    for (const auto& transition : prepared_states_[sid].transitions_) {
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

  for (size_t state_id = 0; state_id < prepared_states_.size(); ++state_id) {
    const auto& state = prepared_states_[state_id];
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
  std::vector<PreparedState> reordered(prepared_states_.size());

  for (StateId old_id = 0; old_id < prepared_states_.size(); ++old_id) {
    StateId new_id = old_to_new[old_id];
    PreparedState& old_state = prepared_states_[old_id];
    for (auto& t : old_state.transitions_) {
      t.target_ = old_to_new[t.target_];
    }
    reordered[new_id] = std::move(old_state);
  }

  start_state_ = old_to_new[start_state_];
  prepared_states_ = std::move(reordered);
}

std::optional<StateId> Dfa::nextState(StateId current, uint8_t input) const {
  const State& state = states_[current];

  // Lookup the transition table
  if (state.transition_table_[input] != std::numeric_limits<State::TableStateID>::max()) {
    return static_cast<StateId>(state.transition_table_[input]);
  }

  return std::nullopt;
}
} // namespace Sregex::Automata