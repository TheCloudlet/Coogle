#include "parser.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string_view>

// TODO: All string functions should review it's performance
std::string_view trim(std::string_view sv) {
  const size_t start = sv.find_first_not_of(" \t\n\r");
  if (start == std::string_view::npos) {
    return {};
  }
  const size_t end = sv.find_last_not_of(" \t\n\r");
  return sv.substr(start, end - start + 1);
}

std::string toString(const Signature &sig) {
  std::string result = sig.retType + "(";

  for (size_t i = 0; i < sig.argType.size(); ++i) {
    result += sig.argType[i];
    if (i != sig.argType.size() - 1)
      result += ", ";
  }

  result += ")";
  return result;
}

std::string normalizeType(std::string_view type) {
  std::string result;
  result.reserve(type.size()); // Pre-allocate to avoid reallocations

  for (size_t i = 0; i < type.size(); ++i) {
    char c = type[i];

    // Skip all whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
      continue;
    }

    // Skip "const" keyword (check for word boundaries)
    if (i + 5 <= type.size() && type.substr(i, 5) == "const") {
      // Check if it's a complete word (not part of another identifier)
      bool isWordStart =
          (i == 0 || std::isspace(static_cast<unsigned char>(type[i - 1])) ||
           type[i - 1] == '*' || type[i - 1] == '&');
      bool isWordEnd = (i + 5 == type.size() ||
                        std::isspace(static_cast<unsigned char>(type[i + 5])) ||
                        type[i + 5] == '*' || type[i + 5] == '&');

      if (isWordStart && isWordEnd) {
        i += 4; // Skip "const" (loop will increment by 1)
        continue;
      }
    }

    result += c;
  }

  return result;
}

bool parseFunctionSignature(std::string_view input, Signature &output) {
  // FIXME: not support function pointer yet
  size_t parenOpen = input.find('(');
  size_t parenClose = input.find(')', parenOpen);

  if (parenOpen == std::string_view::npos ||
      parenClose == std::string_view::npos || parenClose <= parenOpen) {
    std::fprintf(stderr, "Invalid function signature: '%.*s'\n",
                 (int)input.size(), input.data());
    return false;
  }

  output.retType = input.substr(0, parenOpen);

  size_t start = parenOpen + 1;
  while (start < input.size()) {
    size_t end = input.find_first_of(",)", start);
    std::string_view token = input.substr(start, end - start);
    std::string_view cleaned = trim(token);
    if (!cleaned.empty()) {
      output.argType.push_back(std::string(cleaned));
    }
    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }

  printf("\nMatched Functions for Signature: %s\n", toString(output).c_str());
  return true;
}

bool isSignatureMatch(const Signature &a, const Signature &b) {
  if (normalizeType(a.retType) != normalizeType(b.retType)) {
    return false;
  }
  if (a.argType.size() != b.argType.size()) {
    return false;
  }

  for (size_t i = 0; i < a.argType.size(); ++i) {
    if (normalizeType(a.argType[i]) != normalizeType(b.argType[i])) {
      return false;
    }
  }

  return true;
}
