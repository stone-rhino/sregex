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

#include <bitset>
#include <cstring>
#include <expected>
#include <memory>
#include <string>
#include <string_view>

#include "hir_kind.h"

namespace Sregex::Parser {
struct Hir {
  // The underlying HIR kind
  HirKind kind;
  // The option settings for this HIR
  std::bitset<static_cast<size_t>(OptionSettingBitOffset::MAX)> option_settings_;

  static std::expected<std::unique_ptr<Hir>, std::string> parse(const std::string& pattern) {
    return parse(pattern.data(), pattern.size());
  }
  static std::expected<std::unique_ptr<Hir>, std::string> parse(std::string_view pattern) {
    return parse(pattern.data(), pattern.size());
  }
  static std::expected<std::unique_ptr<Hir>, std::string> parse(const char* pattern) {
    return parse(pattern, std::strlen(pattern));
  }
  static std::expected<std::unique_ptr<Hir>, std::string> parse(const char* pattern, size_t length);
};
} // namespace Sregex::Parser