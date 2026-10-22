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

#include <codecvt>
#include <format>
#include <vector>

#include <assert.h>

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
  size_t longest_path = 0;
  for (const auto& pattern : patterns) {
    auto sub_nfa = withPattern(pattern);
    if (!sub_nfa.ok()) {
      Nfa nfa;
      nfa.err_ = sub_nfa.err();
      return nfa;
    }
    longest_path = std::max(longest_path, sub_nfa.longest_path_);
    nfas.emplace_back(std::move(sub_nfa));
  }

  if (nfas.size() == 1) {
    return std::move(nfas[0]);
  } else {
    Nfa nfa = withAlternation(nfas);
    nfa.longest_path_ = longest_path;
    return nfa;
  }
}

Nfa NfaBuilder::withHir(const Parser::Hir& hir) {
  using namespace Parser;

  Nfa nfa;

  std::visit(
      [&](auto&& kind) {
        using T = std::decay_t<decltype(kind)>;
        if constexpr (std::is_same_v<T, LiteralKind>) {
          nfa = withString(kind);
        } else if constexpr (std::is_same_v<T, UnicodeClassKind>) {
          std::vector<Nfa> nfas;
          for (const auto& range : kind.ranges_) {
            nfas.emplace_back(withRange(range.start_, range.end_));
          }
          if (nfas.size() == 1) {
            nfa = std::move(nfas[0]);
          } else {
            nfa = withAlternation(nfas);
          }
        } else if constexpr (std::is_same_v<T, AnchorKind>) {
          nfa = withAnchor(kind);
        } else if constexpr (std::is_same_v<T, RepetitionKind>) {
          nfa = withHir(*kind.sub_);
          if (!nfa.ok()) {
            return;
          }

          auto result = withRangeRepetition(nfa, kind.min_, kind.max_);
          if (!result) {
            nfa.err_ = result.error();
            return;
          }
          nfa = std::move(result.value());
        } else if constexpr (std::is_same_v<T, CaptureKind>) {
          nfa = withHir(*kind.sub_);
        } else if constexpr (std::is_same_v<T, ConcatKind>) {
          for (const auto& sub : kind) {
            Nfa sub_nfa = withHir(*sub);
            if (!sub_nfa.ok()) {
              nfa.err_ = sub_nfa.err();
              return;
            }
            nfa = withConcat(nfa, sub_nfa);
          }
        } else if constexpr (std::is_same_v<T, AlternationKind>) {
          std::vector<Nfa> nfas;
          for (const auto& sub : kind) {
            Nfa sub_nfa = withHir(*sub);
            if (!sub_nfa.ok()) {
              nfa.err_ = sub_nfa.err();
              return;
            }
            nfas.emplace_back(std::move(sub_nfa));
          }
          nfa = withAlternation(nfas);
        }
      },
      hir.kind);

  if (nfa.ok()) {
    nfa.longest_path_ = longestPathFromHir(hir);
  }

  return nfa;
}

Nfa NfaBuilder::withAnchor(Parser::AnchorKind anchor) {
  Nfa nfa;
  // FIXME(zhouyu 2026-10-10): Implement support for anchors such as ^ and $ in the NFA.
  nfa.err_ = "Not supported anchor";
  return nfa;
}

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

Nfa NfaBuilder::withRange(uint32_t start_unicode, uint32_t end_unicode) {
  std::vector<Seq> ranges = utf8Ranges(start_unicode, end_unicode);
  std::vector<Nfa> sub_nfas;
  for (const auto& seq : ranges) {
    Nfa sub_nfa;
    sub_nfa.states_.resize(seq.size() + 1);
    sub_nfa.start_state_ = NEW_DEFAULT_START_STATE;
    sub_nfa.accept_state_ = static_cast<StateId>(seq.size());
    for (size_t i = 0; i < seq.size(); ++i) {
      sub_nfa.states_[i].addTransition(seq[i].first, seq[i].second, static_cast<StateId>(i + 1));
    }
    sub_nfas.push_back(std::move(sub_nfa));
  }

  if (sub_nfas.size() > 1) {
    return withAlternation(sub_nfas);
  } else {
    return std::move(sub_nfas.front());
  }
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

  // Add all states from the second NFA to the new NFA, adjusting the target state IDs for
  // transitions
  appendStatesWithOffset(nfa, second, first.states_.size());

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

  // Add all states from each NFA to the new NFA, adjusting the target state IDs for transitions
  std::vector<StateId> sub_accept_states;
  sub_accept_states.reserve(nfas.size());
  offset = nfa.states_.size();
  for (const auto& sub_nfa : nfas) {
    size_t sub_accept = sub_nfa.accept_state_ + nfa.states_.size();
    sub_accept_states.push_back(sub_accept);
    appendStatesWithOffset(nfa, sub_nfa, offset);
    offset += sub_nfa.states_.size();
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

  // Add all states from the base_nfa to new NFA, adjusting the target state IDs for transitions
  appendStatesWithOffset(nfa, base_nfa, nfa.states_.size());

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

  // Create a new start state that has epsilon transitions to the start state of the base_nfa and
  // to the new accept state. The epsilon transition to the new accept will be added after the NFA
  // (a) has been processed, allowing for zero occurrences of the pattern
  nfa.start_state_ = NEW_DEFAULT_START_STATE;
  nfa.states_.reserve(base_nfa.states_.size() + 2); // +2 for the new start and accept state
  nfa.states_.emplace_back();
  nfa.states_.front().addEpsilon(base_nfa.start_state_ + 1); // +1 for the new start state

  // Add all states from the base_nfa to the new NFA
  appendStatesWithOffset(nfa, base_nfa, nfa.states_.size());

  // Create a new accept state that the base_nfa will transition to via an epsilon transition, and
  // add an epsilon transition from the new start state to this new accept state, allowing for
  // zero or one occurrences of the pattern.
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

void NfaBuilder::appendStatesWithOffset(Nfa& dst_nfa, const Nfa& src_nfa, size_t offset) {
  for (const auto& state : src_nfa.states_) {
    Nfa::State new_state = state;
    for (auto& transition : new_state.transitions_) {
      transition.target_ += offset;
    }
    for (auto& eid : new_state.epsilons_) {
      eid += offset;
    }
    dst_nfa.states_.emplace_back(std::move(new_state));
  }
}

std::vector<NfaBuilder::Seq> NfaBuilder::utf8Ranges(uint32_t start_unicode, uint32_t end_unicode) {
  std::vector<Seq> ranges;

  static const std::pair<uint32_t, uint32_t> kUtf8Boundaries[] = {
      {0x0000, 0x007F},    // 1 byte
      {0x0080, 0x07FF},    // 2 bytes
      {0x0800, 0xD7FF},    // 3 bytes (before surrogate area)
      {0xE000, 0xFFFF},    // 3 bytes (after surrogate area)
      {0x10000, 0x10FFFF}, // 4 bytes
  };

  static std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;

  for (auto [range_start, range_end] : kUtf8Boundaries) {
    uint32_t start = std::max(start_unicode, range_start);
    uint32_t end = std::min(end_unicode, range_end);
    if (start <= end) {
      auto start_utf8_bytes = conv.to_bytes(start);
      auto end_utf8_bytes = conv.to_bytes(end);
      splitByteRange({start_utf8_bytes.begin(), start_utf8_bytes.end()},
                     {end_utf8_bytes.begin(), end_utf8_bytes.end()}, ranges);
    }
  }

  return ranges;
}

void NfaBuilder::splitByteRange(const std::vector<unsigned char>& start,
                                const std::vector<unsigned char>& end, std::vector<Seq>& ranges) {
  const size_t byte_count = start.size();
  assert(byte_count != 0);
  assert(byte_count == end.size());
  if (byte_count == 0 || byte_count != end.size()) {
    return;
  }

  // Find the first byte where start and end differ
  size_t diff_index = 0;
  for (size_t i = 0; i < byte_count; ++i) {
    if (start[i] == end[i]) {
      ++diff_index;
    } else {
      break;
    }
  }

  // Case 1: All bytes are the same, the range same as the start bytes
  {
    if (diff_index == byte_count) {
      Seq seq;
      seq.reserve(byte_count);
      for (size_t i = 0; i < byte_count; ++i) {
        seq.push_back({start[i], start[i]});
      }
      ranges.push_back(std::move(seq));
      return;
    }
  }

  // Case 2: Only the last byte differs, no need to split further
  {
    if (diff_index == byte_count - 1) {
      Seq seq;
      seq.reserve(byte_count);
      for (size_t i = 0; i < byte_count; ++i) {
        if (i == byte_count - 1) {
          seq.push_back({start[i], end[i]});
        } else {
          seq.push_back({start[i], start[i]});
        }
      }
      ranges.push_back(std::move(seq));
      return;
    }
  }

  // Case 3: More than the last byte differs, Need to split into left, middle, and right ranges
  {
    // Common prefix
    Seq prefix;
    prefix.reserve(diff_index);
    for (size_t i = 0; i < diff_index; ++i) {
      prefix.push_back({start[i], start[i]});
    }

    // Left range
    {
      bool left_full = true;
      for (size_t i = diff_index + 1; i < byte_count; ++i) {
        if (start[i] != 0x80) {
          left_full = false;
          break;
        }
      }

      if (left_full) {
        Seq seq = prefix;
        seq.push_back({start[diff_index], start[diff_index]});
        for (size_t i = diff_index + 1; i < byte_count; ++i) {
          seq.push_back({0x80, 0xBF});
        }
        ranges.push_back(std::move(seq));
      } else {
        // Enumerate all possible positions to relax the byte range in the left sequence
        for (size_t relax_pos = diff_index + 1; relax_pos < byte_count; ++relax_pos) {
          Seq seq = prefix;
          seq.push_back({start[diff_index], start[diff_index]});

          // Before the relax pos, the bytes are fixed as in the start sequence
          for (size_t before_relax = diff_index + 1; before_relax < relax_pos; ++before_relax) {
            seq.push_back({start[before_relax], start[before_relax]});
          }

          // The relax pos byte range
          if (relax_pos < byte_count - 1) {
            if (start[relax_pos] >= 0xBF) {
              continue;
            }
            seq.push_back({start[relax_pos] + 1, 0xBF});
          } else {
            seq.push_back({start[relax_pos], 0xBF});
          }

          // After the relax pos, the bytes are full range [0x80, 0xBF]
          for (size_t after_relax = relax_pos + 1; after_relax < byte_count; ++after_relax) {
            seq.push_back({0x80, 0xBF});
          }
          ranges.push_back(std::move(seq));
        }
      }
    }

    // Middle range
    {
      // The middle range is non-empty only if start[diff_index] + 1 <= end[diff_index] - 1
      if (static_cast<int>(start[diff_index]) + 1 <= static_cast<int>(end[diff_index]) - 1) {
        Seq seq = prefix;
        seq.push_back({start[diff_index] + 1, end[diff_index] - 1});
        for (size_t i = diff_index + 1; i < byte_count; ++i) {
          seq.push_back({0x80, 0xBF});
        }
        ranges.push_back(std::move(seq));
      }
    }

    // Right range
    {
      bool right_full = true;
      for (size_t i = diff_index + 1; i < byte_count; ++i) {
        if (end[i] != 0xBF) {
          right_full = false;
          break;
        }
      }

      if (right_full) {
        Seq seq = prefix;
        seq.push_back({(end[diff_index]), end[diff_index]});
        for (size_t i = diff_index + 1; i < byte_count; ++i) {
          seq.push_back({0x80, 0xBF});
        }
        ranges.push_back(std::move(seq));
      } else {
        // Enumerate all possible positions to relax the byte range in the right sequence
        for (size_t relax_pos = diff_index + 1; relax_pos < byte_count; ++relax_pos) {
          Seq seq = prefix;
          seq.push_back({end[diff_index], end[diff_index]});

          // Before the relax pos, the bytes are fixed as in the end sequence
          for (size_t before_relax = diff_index + 1; before_relax < relax_pos; ++before_relax) {
            seq.push_back({end[before_relax], end[before_relax]});
          }

          // The relax pos byte range
          if (relax_pos < byte_count - 1) {
            if (end[relax_pos] <= 0x80) {
              continue;
            }
            seq.push_back({0x80, end[relax_pos] - 1});
          } else {
            seq.push_back({0x80, end[relax_pos]});
          }

          // After the relax pos, the bytes are full range [0x80, 0xBF]
          for (size_t k = relax_pos + 1; k < byte_count; ++k) {
            seq.push_back({0x80, 0xBF});
          }
          ranges.push_back(std::move(seq));
        }
      }
    }
  }
}

size_t NfaBuilder::longestPathFromHir(const Parser::Hir& hir) {
  using namespace Parser;

  return std::visit(
      [&](auto&& kind) -> size_t {
        using T = std::decay_t<decltype(kind)>;
        if constexpr (std::is_same_v<T, EmptyKind>) {
          return 0;
        } else if constexpr (std::is_same_v<T, LiteralKind>) {
          return kind.size();
        } else if constexpr (std::is_same_v<T, UnicodeClassKind>) {
          size_t best = 0;
          for (const auto& range : kind.ranges_) {
            best = std::max(best, utf8ByteLength(range.start_));
            best = std::max(best, utf8ByteLength(range.end_));
          }
          return best;
        } else if constexpr (std::is_same_v<T, AnchorKind>) {
          return 0;
        } else if constexpr (std::is_same_v<T, RepetitionKind>) {
          const size_t unit = longestPathFromHir(*kind.sub_);
          if (kind.max_.has_value()) {
            return unit * kind.max_.value();
          }
          if (kind.min_ == 0) {
            return unit;
          }
          if (kind.min_ == 1) {
            return unit;
          }
          return unit * (kind.min_ + 1);
        } else if constexpr (std::is_same_v<T, CaptureKind>) {
          return longestPathFromHir(*kind.sub_);
        } else if constexpr (std::is_same_v<T, ConcatKind>) {
          size_t total = 0;
          for (const auto& sub : kind) {
            total += longestPathFromHir(*sub);
          }
          return total;
        } else if constexpr (std::is_same_v<T, AlternationKind>) {
          size_t best = 0;
          for (const auto& sub : kind) {
            best = std::max(best, longestPathFromHir(*sub));
          }
          return best;
        }
      },
      hir.kind);
}

size_t NfaBuilder::utf8ByteLength(uint32_t codepoint) {
  if (codepoint <= 0x7F) {
    return 1;
  }
  if (codepoint <= 0x7FF) {
    return 2;
  }
  if (codepoint <= 0xFFFF) {
    return 3;
  }
  return 4;
}

} // namespace Sregex::Automata