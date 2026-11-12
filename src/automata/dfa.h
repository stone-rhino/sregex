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
#include <optional>
#include <span>
#include <unordered_set>
#include <vector>

#include "automaton.h"
#include "transition.h"

namespace Sregex {
enum class PatternKind : uint8_t;
namespace Automata {
class Dfa : public Automaton<Dfa> {
  friend class Automaton<Dfa>;
  friend class boost::serialization::access;

public:
  Dfa() = default;
  Dfa(const std::string& pattern, PatternKind kind);
  Dfa(const std::span<const std::string>& patterns, PatternKind kind);
  Dfa(const std::span<const std::string>& patterns, const std::span<const PatternKind>& kinds);
  Dfa(const std::vector<std::vector<Transition>>& transitions,
      const std::unordered_set<StateId>& accept_ids, size_t longest_path);
  Dfa(const Dfa& other) = delete;
  Dfa& operator=(const Dfa& other) = delete;
  Dfa(Dfa&& other) = default;
  Dfa& operator=(Dfa&& other) = default;

  // Avoid the boost serialization function hiding the base class serialize function
  using Automaton<Dfa>::serialize;

protected:
  // Implementation of the isMatch interface
  bool isMatchImpl(const char* input, size_t length) const;

  // Implementation of the isPartialMatch interface
  bool isPartialMatchImpl(const char* input, size_t length, bool is_soft) const;

  // Implementation of the toDot interface
  std::string toDotImpl() const;

private:
  // Normalize the DFA by sorting the transitions of each state.
  void normalize();

  // Use partition refinement(Moore) to minimize the DFA.
  void minimize();

  // Reorder states to improve locality in the state array.
  // This keeps the language recognized by the DFA unchanged while remapping state IDs to place
  // traversal-near states closer in memory.
  void reorderStatesForLocality();
  std::vector<StateId> buildReorderedMapBFS() const;
  std::vector<StateId> buildReorderedMapDFS() const;
  struct ReorderMetrics;
  void computeReorderMetrics(const std::vector<StateId>& reordered_map,
                             ReorderMetrics& metrics) const;
  void applyReorderedMap(const std::vector<StateId>& reordered_map);

  // Get the next state for the given input character from the current state, if it exists.
  std::optional<StateId> nextState(StateId current, uint8_t input) const;

  template <class Archive> void serialize(Archive& ar, const unsigned int version) {
    ar & states_;
    ar & start_state_;
    ar & bfs_metrics_;
    ar & dfs_metrics_;
  }

private:
  enum class StateType : uint8_t {
    // Not an accept state
    Normal,
    // Accept state that has no outgoing transitions
    AcceptTerminal,
    // Accept state that has outgoing transitions
    AcceptContinuable
  };

  struct PreparedState {
    StateType state_type_ = StateType::Normal;
    std::vector<Transition> transitions_;
    bool isAcceptState() const { return state_type_ != StateType::Normal; }
  };

  struct State {
    StateType state_type_ = StateType::Normal;

    // Each entry is a TableStateID representing the target state ID
    using TableStateID = uint32_t;
    std::array<TableStateID, 256> transition_table_;

    State() = default;
    State(const PreparedState& prepared) {
      state_type_ = prepared.state_type_;
      transition_table_.fill(std::numeric_limits<TableStateID>::max());
      for (const auto& t : prepared.transitions_) {
        for (auto c = t.start_; c <= t.end_; ++c) {
          transition_table_[c] = t.target_;
        }
      }
    }
    bool isAcceptState() const { return state_type_ != StateType::Normal; }

    template <class Archive> void serialize(Archive& ar, const unsigned int version) {
      ar & state_type_;
      ar & transition_table_;
    }
  };

  struct ReorderMetrics {
    size_t edge_count_ = 0;
    size_t total_abs_delta_ = 0;
    double avg_abs_delta_ = 0;
    size_t p50_abs_delta_ = 0;
    size_t p90_abs_delta_ = 0;
    size_t max_abs_delta_ = 0;
    double adjacent_ratio_ = 0;
    double near2_ratio_ = 0;
    double near4_ratio_ = 0;
    double long8_ratio_ = 0;
    size_t start_edge_count_ = 0;
    double start_avg_abs_delta_ = 0;

    template <class Archive> void serialize(Archive& ar, const unsigned int version) {
      ar & edge_count_;
      ar & total_abs_delta_;
      ar & avg_abs_delta_;
      ar & p50_abs_delta_;
      ar & p90_abs_delta_;
      ar & max_abs_delta_;
      ar & adjacent_ratio_;
      ar & near2_ratio_;
      ar & near4_ratio_;
      ar & long8_ratio_;
      ar & start_edge_count_;
      ar & start_avg_abs_delta_;
    }
  };

private:
  std::vector<PreparedState> prepared_states_;
  std::vector<State> states_;
  StateId start_state_;
  ReorderMetrics bfs_metrics_;
  ReorderMetrics dfs_metrics_;
};
} // namespace Automata
} // namespace Sregex