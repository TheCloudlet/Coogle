// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Implementation of function signature parser, type normalizer, and
// signature matcher with zero-allocation design.

#include "coogle/parser.h"

#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <iostream>
#include <string_view>

namespace coogle {

namespace {
// Helper to trim whitespace from string_view
std::string_view trim(std::string_view Sv) {
  const size_t Start = Sv.find_first_not_of(" \t\n\r");
  if (Start == std::string_view::npos) {
    return {};
  }
  const size_t End = Sv.find_last_not_of(" \t\n\r");
  return Sv.substr(Start, End - Start + 1);
}

// Keywords to skip during type normalization
constexpr std::array<std::string_view, 4> Keywords = {"const", "class",
                                                      "struct", "union"};

// Helper to check and skip a keyword at the current position
bool skipKeyword(std::string_view Type, size_t &Idx) {
  for (auto Keyword : Keywords) {
    if (Type.size() >= Idx + Keyword.size() &&
        Type.substr(Idx, Keyword.size()) == Keyword) {
      // Check word boundaries
      bool IsStart = (Idx == 0 ||
                      std::isspace(static_cast<unsigned char>(Type[Idx - 1])) ||
                      std::ispunct(static_cast<unsigned char>(Type[Idx - 1])));
      bool IsEnd =
          (Idx + Keyword.size() == Type.size() ||
           std::isspace(
               static_cast<unsigned char>(Type[Idx + Keyword.size()])) ||
           std::ispunct(
               static_cast<unsigned char>(Type[Idx + Keyword.size()])));
      if (IsStart && IsEnd) {
        Idx += Keyword.size() - 1; // -1 because loop will increment
        return true;
      }
    }
  }
  return false;
}

// Helper to normalize std::basic_string<...> to std::string in-place
size_t normalizeStdString(coogle::span<char> Buffer, size_t WriteIdx) {
  // Convert span to string_view for searching
  std::string_view BufferView(Buffer.data(), WriteIdx);
  size_t Pos = BufferView.find("std::basic_string");

  if (Pos == std::string_view::npos) {
    return WriteIdx;
  }

  // Find template start
  size_t StartTemplate = BufferView.find('<', Pos);
  if (StartTemplate == std::string_view::npos) {
    return WriteIdx;
  }

  // Find matching closing bracket
  int BracketLevel = 1;
  size_t EndTemplate = StartTemplate + 1;
  while (EndTemplate < WriteIdx && BracketLevel > 0) {
    if (Buffer[EndTemplate] == '<') {
      BracketLevel++;
    } else if (Buffer[EndTemplate] == '>') {
      BracketLevel--;
    }
    EndTemplate++;
  }

  if (BracketLevel != 0) {
    return WriteIdx; // Malformed, leave as-is
  }

  // Replace "std::basic_string<...>" with "std::string"
  constexpr std::string_view Replacement = "std::string";
  const size_t OldLen = EndTemplate - Pos;
  const size_t NewLen = Replacement.size();

  if (NewLen < OldLen) {
    // Copy replacement
    std::copy(Replacement.begin(), Replacement.end(), Buffer.begin() + Pos);
    // Shift rest of buffer left
    std::copy(Buffer.begin() + EndTemplate, Buffer.begin() + WriteIdx,
              Buffer.begin() + Pos + NewLen);
    WriteIdx -= (OldLen - NewLen);
  } else if (NewLen > OldLen) {
    // Need to shift right - but our buffer should be large enough
    std::copy_backward(Buffer.begin() + EndTemplate, Buffer.begin() + WriteIdx,
                       Buffer.begin() + WriteIdx + (NewLen - OldLen));
    std::copy(Replacement.begin(), Replacement.end(), Buffer.begin() + Pos);
    WriteIdx += (NewLen - OldLen);
  } else {
    // Same size, just copy
    std::copy(Replacement.begin(), Replacement.end(), Buffer.begin() + Pos);
  }

  return WriteIdx;
}

} // anonymous namespace

std::string_view normalizeType(StringArena &Arena, std::string_view Type) {
  // Allocate buffer for normalized type (worst case: same size as input)
  coogle::span<char> Buffer =
      Arena.allocate(Type.size() * 2); // Extra space for safety
  size_t WriteIdx = 0;

  // First pass: remove whitespace and keywords
  for (size_t ReadIdx = 0; ReadIdx < Type.size(); ++ReadIdx) {
    // Skip whitespace
    if (std::isspace(static_cast<unsigned char>(Type[ReadIdx]))) {
      continue;
    }

    // Skip keywords
    if (skipKeyword(Type, ReadIdx)) {
      continue;
    }

    Buffer[WriteIdx++] = Type[ReadIdx];
  }

  // Second pass: normalize std::basic_string to std::string
  WriteIdx = normalizeStdString(Buffer, WriteIdx);

  // Finalize with actual size
  return Arena.finalize(Buffer, WriteIdx);
}

std::optional<Signature> parseFunctionSignature(SignatureStorage &Storage,
                                                std::string_view Input) {
  // Find opening parenthesis
  size_t ParenOpen = Input.find('(');
  if (ParenOpen == std::string_view::npos) {
    std::cerr << fmt::format("Invalid function signature (missing '('): '{}'\n",
                             Input);
    return std::nullopt;
  }

  // Find matching closing parenthesis
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

  // Parse and store return type (original and normalized)
  std::string_view RetTypeSV = trim(Input.substr(0, ParenOpen));
  Result.RetType = Storage.internString(RetTypeSV);
  Result.RetTypeNorm = normalizeType(Storage.arena(), Result.RetType);

  // Parse arguments
  std::string_view ArgSV =
      Input.substr(ParenOpen + 1, ParenClose - (ParenOpen + 1));
  if (trim(ArgSV).empty()) {
    // No arguments - empty spans
    Result.ArgTypes = span<std::string_view>{};
    Result.ArgTypesNorm = span<std::string_view>{};
    return Result;
  }

  // Count arguments first (for pre-allocation)
  size_t ArgCount = 1;
  int Level = 0;
  for (size_t i = 0; i < ArgSV.length(); ++i) {
    if (ArgSV[i] == '(' || ArgSV[i] == '<') {
      Level++;
    } else if (ArgSV[i] == ')' || ArgSV[i] == '>') {
      Level--;
    } else if (ArgSV[i] == ',' && Level == 0) {
      ArgCount++;
    }
  }

  // Pre-allocate storage
  Storage.reserveArgs(ArgCount);

  // Parse and store arguments
  size_t Start = 0;
  Level = 0;
  for (size_t i = 0; i < ArgSV.length(); ++i) {
    if (ArgSV[i] == '(' || ArgSV[i] == '<') {
      Level++;
    } else if (ArgSV[i] == ')' || ArgSV[i] == '>') {
      Level--;
    } else if (ArgSV[i] == ',' && Level == 0) {
      std::string_view Token = trim(ArgSV.substr(Start, i - Start));
      if (!Token.empty()) {
        std::string_view ArgOrig = Storage.internString(Token);
        std::string_view ArgNorm = normalizeType(Storage.arena(), ArgOrig);
        Storage.addArg(ArgOrig, ArgNorm);
      }
      Start = i + 1;
    }
  }

  // Last argument
  std::string_view Token = trim(ArgSV.substr(Start));
  if (!Token.empty()) {
    std::string_view ArgOrig = Storage.internString(Token);
    std::string_view ArgNorm = normalizeType(Storage.arena(), ArgOrig);
    Storage.addArg(ArgOrig, ArgNorm);
  }

  Result.ArgTypes = Storage.getArgs();
  Result.ArgTypesNorm = Storage.getArgsNorm();

  return Result;
}

std::string toString(const Signature &Sig) {
  // Note: Still returns std::string for display purposes
  return fmt::format("{}({})", Sig.RetType, fmt::join(Sig.ArgTypes, ", "));
}

bool isSignatureMatch(const Signature &UserSig, const Signature &ActualSig) {
  // Direct comparison using pre-normalized types
  if (UserSig.RetTypeNorm != ActualSig.RetTypeNorm) {
    return false;
  }

  if (UserSig.ArgTypesNorm.size() != ActualSig.ArgTypesNorm.size()) {
    return false;
  }

  // Check each argument (with wildcard support)
  for (size_t i = 0; i < UserSig.ArgTypesNorm.size(); ++i) {
    // Wildcard matches any type
    if (UserSig.ArgTypes[i] == "*") {
      continue;
    }

    if (UserSig.ArgTypesNorm[i] != ActualSig.ArgTypesNorm[i]) {
      return false;
    }
  }

  return true;
}

} // namespace coogle
