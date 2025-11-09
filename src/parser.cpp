#include "parser.h"

#include <cctype>
#include <cstddef>
#include <format>
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
  std::string Result = Sig.RetType + "(";

  for (size_t ArgIdx = 0; ArgIdx < Sig.ArgType.size(); ++ArgIdx) {
    Result += Sig.ArgType[ArgIdx];
    if (ArgIdx != Sig.ArgType.size() - 1)
      Result += ", ";
  }

  Result += ")";
  return Result;
}

std::string normalizeType(std::string_view Type) {
  std::string Result;
  Result.reserve(Type.size()); // Pre-allocate to avoid reallocations

  for (size_t Idx = 0; Idx < Type.size(); ++Idx) {
    char C = Type[Idx];

    // Skip all whitespace
    if (std::isspace(static_cast<unsigned char>(C))) {
      continue;
    }

    // Skip "const" keyword (check for word boundaries)
    if (Idx + 5 <= Type.size() && Type.substr(Idx, 5) == "const") {
      // Check if it's a complete word (not part of another identifier)
      bool IsWordStart =
          (Idx == 0 ||
           std::isspace(static_cast<unsigned char>(Type[Idx - 1])) ||
           Type[Idx - 1] == '*' || Type[Idx - 1] == '&');
      bool IsWordEnd =
          (Idx + 5 == Type.size() ||
           std::isspace(static_cast<unsigned char>(Type[Idx + 5])) ||
           Type[Idx + 5] == '*' || Type[Idx + 5] == '&');

      if (IsWordStart && IsWordEnd) {
        Idx += 4; // Skip "const" (loop will increment by 1)
        continue;
      }
    }

    Result += C;
  }

  return Result;
}

bool parseFunctionSignature(std::string_view Input, Signature &Output) {
  // FIXME: not support function pointer yet
  size_t ParenOpen = Input.find('(');
  size_t ParenClose = Input.find(')', ParenOpen);

  if (ParenOpen == std::string_view::npos ||
      ParenClose == std::string_view::npos || ParenClose <= ParenOpen) {
    std::cerr << std::format("Invalid function signature: '{}'\n", Input);
    return false;
  }

  Output.RetType = Input.substr(0, ParenOpen);

  size_t Start = ParenOpen + 1;
  while (Start < Input.size()) {
    size_t End = Input.find_first_of(",)", Start);
    std::string_view Token = Input.substr(Start, End - Start);
    std::string_view Cleaned = trim(Token);
    if (!Cleaned.empty()) {
      Output.ArgType.push_back(std::string(Cleaned));
    }
    if (End == std::string_view::npos) {
      break;
    }
    Start = End + 1;
  }

  std::cout << std::format("\nMatched Functions for Signature: {}\n",
                           toString(Output));
  return true;
}

bool isSignatureMatch(const Signature &A, const Signature &B) {
  if (normalizeType(A.RetType) != normalizeType(B.RetType)) {
    return false;
  }
  if (A.ArgType.size() != B.ArgType.size()) {
    return false;
  }

  for (size_t ArgIdx = 0; ArgIdx < A.ArgType.size(); ++ArgIdx) {
    if (normalizeType(A.ArgType[ArgIdx]) != normalizeType(B.ArgType[ArgIdx])) {
      return false;
    }
  }

  return true;
}
