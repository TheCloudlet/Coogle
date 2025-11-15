#pragma once
#include <string_view>

namespace coogle::colors {

// ANSI Color Codes
constexpr std::string_view Reset = "\033[0m";
constexpr std::string_view Bold = "\033[1m";
constexpr std::string_view Green = "\033[32m";
constexpr std::string_view Yellow = "\033[33m";
constexpr std::string_view Blue = "\033[34m";
constexpr std::string_view Cyan = "\033[36m";
constexpr std::string_view Grey = "\033[90m";

} // namespace coogle::colors
