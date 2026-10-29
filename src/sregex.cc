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
#include "sregex.h"

#include "automata/dfa.h"

namespace Sregex {
Sregex::Sregex(const std::string& pattern) { dfa_ = std::make_unique<Automata::Dfa>(pattern); }

Sregex::Sregex(const std::span<const std::string>& patterns) {
  dfa_ = std::make_unique<Automata::Dfa>(patterns);
}

Sregex::~Sregex() = default;

bool Sregex::ok() const { return dfa_->ok(); }

const std::string& Sregex::err() const { return dfa_->err(); }

bool Sregex::isMatch(const char* input, size_t length) const {
  return dfa_->isMatch(input, length);
}

bool Sregex::isPartialMatch(const char* input, size_t length, bool is_soft) const {
  return dfa_->isPartialMatch(input, length, is_soft);
}

} // namespace Sregex