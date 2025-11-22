#include "coogle/parser.h"

#include <cassert> // Added for assert
#include <cctype>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <string_view>

std::string_view trim(std::string_view sv) {
  const size_t Start = sv.find_first_not_of(" \t\n\r");
  if (Start == std::string_view::npos) {
    return {};
  }
  const size_t End = sv.find_last_not_of(" \t\n\r");
  return sv.substr(Start, End - Start + 1);
}

std::string toString(const Signature &Sig) {
  return fmt::format("{}({})", Sig.RetType, fmt::join(Sig.ArgType, ", "));
}

std::string normalizeType(std::string_view Type) {
  std::string Temp;
  Temp.reserve(Type.size()); // Pre-allocate to avoid reallocations

  for (size_t Idx = 0; Idx < Type.size(); ++Idx) {
    // Skip all whitespace
    if (std::isspace(static_cast<unsigned char>(Type[Idx]))) {
      continue;
    }

    // Helper to check for and skip keywords
    auto skip_keyword = [&](const std::string &keyword) {
      if (Type.size() >= Idx + keyword.size() &&
          Type.substr(Idx, keyword.size()) == keyword) {
        bool is_start = (Idx == 0 || std::isspace(Type[Idx - 1]) ||
                         std::ispunct(Type[Idx - 1]));
        bool is_end = (Idx + keyword.size() == Type.size() ||
                       std::isspace(Type[Idx + keyword.size()]) ||
                       std::ispunct(Type[Idx + keyword.size()]));
        if (is_start && is_end) {
          Idx += keyword.size() - 1; // Adjust for loop increment
          return true;
        }
      }
      return false;
    };

    if (skip_keyword("const") || skip_keyword("class") ||
        skip_keyword("struct") || skip_keyword("union")) {
      continue;
    }

    Temp += Type[Idx];
  }

  // Special handling for std::string, which clang expands to a template
  size_t Pos = Temp.find("std::basic_string");
  if (Pos != std::string::npos) {
    size_t StartTemplate = Temp.find('<', Pos);
    if (StartTemplate != std::string::npos) {
      int BracketLevel = 1;
      size_t EndTemplate = StartTemplate + 1;
      while (EndTemplate < Temp.length() && BracketLevel > 0) {
        if (Temp[EndTemplate] == '<') {
          BracketLevel++;
        } else if (Temp[EndTemplate] == '>') {
          BracketLevel--;
        }
        EndTemplate++;
      }

      if (BracketLevel == 0) {
        Temp.replace(Pos, EndTemplate - Pos, "std::string");
      }
    }
  }

  return Temp;
}

std::optional<Signature> parseFunctionSignature(std::string_view Input) {
  size_t ParenOpen = Input.find('(');
  if (ParenOpen == std::string_view::npos) {
    std::cerr << fmt::format("Invalid function signature (missing '('): '{}'\n",
                             Input);
    return std::nullopt;
  }

  // Find matching closing parenthesis, considering nested templates
  int ParenLevel = 0;
  size_t ParenClose = std::string_view::npos;
  for (size_t i = ParenOpen; i < Input.length(); ++i) {
    if (Input[i] == '(') {
      ParenLevel++;
    } else if (Input[i] == ')') {
      ParenLevel--;
      if (ParenLevel == 0) {
        ParenClose = i;
        break;
      }
    }
  }

  if (ParenClose == std::string_view::npos) {
    std::cerr << fmt::format(
        "Invalid function signature (mismatched parentheses): '{}'\n", Input);
    return std::nullopt;
  }

  Signature Result;
  Result.RetType = std::string(trim(Input.substr(0, ParenOpen)));

  // Parse arguments
  std::string_view ArgSV =
      Input.substr(ParenOpen + 1, ParenClose - (ParenOpen + 1));
  if (trim(ArgSV).empty()) {
    // No arguments
    return Result;
  }

  size_t Start = 0;
  int Level = 0; // For parentheses and angle brackets
  for (size_t i = 0; i < ArgSV.length(); ++i) {
    if (ArgSV[i] == '(' || ArgSV[i] == '<') {
      Level++;
    } else if (ArgSV[i] == ')' || ArgSV[i] == '>') {
      Level--;
    } else if (ArgSV[i] == ',' && Level == 0) {
      std::string_view Token = ArgSV.substr(Start, i - Start);
      std::string_view Cleaned = trim(Token);
      if (!Cleaned.empty()) {
        Result.ArgType.push_back(std::string(Cleaned));
      }
      Start = i + 1;
    }
  }

  // Add the last (or only) argument
  std::string_view Token = ArgSV.substr(Start);
  std::string_view Cleaned = trim(Token);
  if (!Cleaned.empty()) {
    Result.ArgType.push_back(std::string(Cleaned));
  }

  return Result;
}

bool isSignatureMatch(const Signature &A, const Signature &B) {
  if (normalizeType(A.RetType) != normalizeType(B.RetType)) {
    return false;
  }
  if (A.ArgType.size() != B.ArgType.size()) {
    return false;
  }

  for (size_t ArgIdx = 0; ArgIdx < A.ArgType.size(); ++ArgIdx) {
    // Handle wildcard argument matching
    if (A.ArgType[ArgIdx] == "*") {
      continue;
    }
    if (normalizeType(A.ArgType[ArgIdx]) != normalizeType(B.ArgType[ArgIdx])) {
      return false;
    }
  }

  return true;
}
