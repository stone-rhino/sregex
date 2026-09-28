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

#include <format>

#include "nfa_builder.h"

namespace Sregex::Automata {
Nfa::Nfa(const std::span<const std::string>& patterns) {
  *this = NfaBuilder::withPatterns(patterns);
}

bool Nfa::isMatchImpl(const char* input, size_t length, bool partial) const {
  throw "Not implemented!";
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

} // namespace Sregex::Automata