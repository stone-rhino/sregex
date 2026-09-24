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
#include "nfa_builder.h"

#include <format>
#include <vector>

namespace Sregex::Automata {
constexpr StateId NEW_DEFAULT_START_STATE = 0;
constexpr StateId NEW_DEFAULT_ACCEPT_STATE = 1;

Nfa NfaBuilder::withPattern(const std::string& pattern) {
  auto result = Parser::Hir::parse(pattern);
  if (!result) {
    Nfa nfa;
    nfa.err_ = result.error();
    return nfa;
  }

  return withHir(*result.value());
}

Nfa NfaBuilder::withPatterns(const std::span<const std::string>& patterns) {
  std::vector<Nfa> nfas;
  for (const auto& pattern : patterns) {
    auto sub_nfa = withPattern(pattern);
    if (!sub_nfa.ok()) {
      Nfa nfa;
      nfa.err_ = sub_nfa.err();
      return nfa;
    }
    nfas.emplace_back(std::move(sub_nfa));
  }

  return withAlternation(nfas);
}

Nfa NfaBuilder::withHir(const Parser::Hir& hir) { throw "Not implemented!"; }

Nfa NfaBuilder::withChar(char c) {
  Nfa nfa;
  nfa.states_.resize(2);
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.accept_state_ = NEW_DEFAULT_ACCEPT_STATE;
  nfa.states_[NEW_DEFAULT_START_STATE].addTransition(c, NEW_DEFAULT_ACCEPT_STATE);

  return nfa;
}

Nfa NfaBuilder::withString(const std::string& str) {
  Nfa nfa;
  if (str.empty()) {
    return nfa;
  }

  nfa.states_.resize(str.size() + 1);
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.accept_state_ = static_cast<StateId>(str.size());
  for (size_t i = 0; i < str.size(); ++i) {
    nfa.states_[i].addTransition(str[i], static_cast<StateId>(i + 1));
  }

  return nfa;
}

Nfa NfaBuilder::withRange(uint32_t start, uint32_t end) {
  Nfa nfa;
  nfa.states_.resize(2);
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.accept_state_ = NEW_DEFAULT_ACCEPT_STATE;
  nfa.states_[NEW_DEFAULT_START_STATE].addTransition(start, end, NEW_DEFAULT_ACCEPT_STATE);

  return nfa;
}

Nfa NfaBuilder::withConcat(const Nfa& first, const Nfa& second) {
  Nfa nfa;
  if (first.states_.empty()) {
    nfa.states_ = second.states_;
    nfa.start_state_ = second.start_state_;
    nfa.accept_state_ = second.accept_state_;
    return nfa;
  }
  if (second.states_.empty()) {
    nfa.states_ = first.states_;
    nfa.start_state_ = first.start_state_;
    nfa.accept_state_ = first.accept_state_;
    return nfa;
  }

  // The start state of the new NFA is the start state of the first NFA
  nfa.start_state_ = first.start_state_;

  // The accept state of the new NFA is the accept state of the second NFA, but we need to adjust
  // its ID
  nfa.accept_state_ = second.accept_state_ + first.states_.size();

  // Add all states from the first NFA  to the new NFA
  nfa.states_.reserve(first.states_.size() + second.states_.size());
  nfa.states_ = first.states_;

  // Add all states from the second NFA to the new NFA
  nfa.states_.insert(nfa.states_.end(), second.states_.begin(), second.states_.end());

  // Add an epsilon transition from the accept state of the first NFA to the start state of the
  // second NFA
  nfa.states_[first.accept_state_].addEpsilon(first.states_.size() + second.start_state_);

  return nfa;
}

Nfa NfaBuilder::withAlternation(const std::span<const Nfa>& nfas) {
  Nfa nfa;
  if (nfas.empty()) {
    return nfa;
  }

  // Reserve space for all states
  size_t total_states = 0;
  for (const auto& n : nfas) {
    total_states += n.states_.size();
  }
  nfa.states_.reserve(total_states + 2); // +2 for the new start and accept state

  // Create a new start state that has epsilon transitions to the start states of each NFA
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.states_.emplace_back(); // new start state
  size_t offset = 1;          // Start from 1 because 0 is the new start state
  auto& new_start_state = nfa.states_.front();
  for (const auto& sub_nfa : nfas) {
    new_start_state.addEpsilon(offset + sub_nfa.start_state_);
    offset += sub_nfa.states_.size();
  }

  // Add all states from each NFA to the new NFA
  std::vector<StateId> sub_accept_states;
  sub_accept_states.reserve(nfas.size());
  for (const auto& sub_nfa : nfas) {
    size_t sub_accept = sub_nfa.accept_state_ + nfa.states_.size();
    sub_accept_states.push_back(sub_accept);
    nfa.states_.insert(nfa.states_.end(), sub_nfa.states_.begin(), sub_nfa.states_.end());
  }

  // Create a new accept state that all NFAs will transition to via epsilon transitions
  nfa.accept_state_ = nfa.states_.size();
  nfa.states_.emplace_back();
  for (auto sub_accept : sub_accept_states) {
    nfa.states_[sub_accept].addEpsilon(nfa.accept_state_);
  }

  return nfa;
}

Nfa NfaBuilder::withKleeneStar(const Nfa& base_nfa) {
  Nfa nfa;
  if (base_nfa.states_.empty()) {
    return nfa;
  }

  // Create a new start state that has epsilon transitions to the start state of the base_nfa
  // and to the new accept state
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.states_.reserve(base_nfa.states_.size() + 2); // +2 for the new start and accept state
  nfa.states_.emplace_back();
  nfa.states_.front().addEpsilon(1 + base_nfa.start_state_);

  // Add all states from the base_nfa to new NFA
  nfa.states_.insert(nfa.states_.end(), base_nfa.states_.begin(), base_nfa.states_.end());

  // Create a new accept state that the base_nfa will transition to via an epsilon transition,
  // and add an epsilon transition from the new start state to this new accept state, and also
  // from the new accept state to new start state, allowing for zero or more occurrences of the
  // pattern
  nfa.accept_state_ = nfa.states_.size();
  nfa.states_.emplace_back();
  nfa.states_[base_nfa.accept_state_ + 1].addEpsilon(nfa.accept_state_);
  nfa.states_.front().addEpsilon(nfa.accept_state_);
  nfa.states_.back().addEpsilon(nfa.start_state_);

  return nfa;
}

Nfa NfaBuilder::withPlus(const Nfa& base_nfa) {
  Nfa nfa;
  if (base_nfa.states_.empty()) {
    return nfa;
  }

  nfa.start_state_ = base_nfa.start_state_;
  nfa.accept_state_ = base_nfa.accept_state_;
  nfa.states_ = base_nfa.states_;
  nfa.states_[nfa.accept_state_].addEpsilon(nfa.start_state_);

  return nfa;
}

Nfa NfaBuilder::withQuestionMark(const Nfa& base_nfa) {
  Nfa nfa;
  if (base_nfa.states_.empty()) {
    return nfa;
  }

  // Create a new start state that has epsilon transitions to the start state of the base_nfa and to
  // the new accept state. The epsilon transition to the new accept will be added after the NFA (a)
  // has been processed, allowing for zero occurrences of the pattern
  nfa.states_.reserve(base_nfa.states_.size() + 2); // +2 for the new start and accept state
  nfa.states_.emplace_back();
  nfa.states_.front().addEpsilon(base_nfa.start_state_ + 1); // +1 for the new start state

  // Add all states from the base_nfa to the new NFA
  nfa.states_.insert(nfa.states_.end(), base_nfa.states_.begin(), base_nfa.states_.end());

  // Create a new accept state that the base_nfa will transition to via an epsilon transition, and
  // add an epsilon transition from the new start state to this new accept state, allowing for zero
  // or one occurrences of the pattern.
  nfa.accept_state_ = nfa.states_.size();
  nfa.states_.emplace_back();
  nfa.states_[base_nfa.accept_state_ + 1].addEpsilon(nfa.accept_state_);
  nfa.states_[nfa.start_state_].addEpsilon(nfa.accept_state_);

  return nfa;
}

Nfa NfaBuilder::withExactRepetition(const Nfa& base_nfa, size_t count) {
  Nfa nfa;
  if (count <= 0) {
    return nfa;
  }

  for (size_t i = 0; i < count; ++i) {
    nfa = withConcat(nfa, base_nfa);
  }

  return nfa;
}

std::expected<Nfa, std::string> NfaBuilder::withRangeRepetition(const Nfa& base_nfa, size_t min,
                                                                std::optional<size_t> max) {
  if (min == 0 && !max.has_value()) {
    return withKleeneStar(base_nfa);
  } else if (min == 1 && !max.has_value()) {
    return withPlus(base_nfa);
  } else if (min == 0 && max.value_or(0) == 1) {
    return withQuestionMark(base_nfa);
  } else if (min > 0 && (!max.has_value() || max.value() >= min)) {
    // For performance reasons, we limit the maximum repetition to a certain threshold.
    // If we want to support larger ranges, we can implement a more efficient algorithm
    // that doesn't require creating a large number of states, maybe using counting, but
    // for now, we will keep it simple and limit the range.
    // TODO: Use counting to support larger ranges without creating a large number of states.
    constexpr size_t MAX_REPETITION_THRESHOLD = 16;

    if (min > MAX_REPETITION_THRESHOLD ||
        (max.has_value() && max.value() > MAX_REPETITION_THRESHOLD)) {
      const std::string max_str = max.has_value() ? std::to_string(max.value()) : "none";
      return std::unexpected(std::format("Repetition range is too large: min={}, max={}. Only "
                                         "ranges with min and max less than {} are supported.",
                                         min, max_str, MAX_REPETITION_THRESHOLD));
    }

    if (max.has_value()) {
      Nfa nfa = withExactRepetition(base_nfa, min);
      Nfa nfa_with_question = withQuestionMark(base_nfa);
      nfa_with_question = withExactRepetition(nfa_with_question, max.value() - min);
      return withConcat(nfa, nfa_with_question);
    } else {
      Nfa nfa = withExactRepetition(base_nfa, min);
      Nfa nfa_with_star = withKleeneStar(base_nfa);
      return withConcat(nfa, nfa_with_star);
    }
  }

  const std::string max_str = max.has_value() ? std::to_string(max.value()) : "none";
  return std::unexpected(std::format("Invalid repetition range: min={}, max={}", min, max_str));
}

} // namespace Sregex::Automata