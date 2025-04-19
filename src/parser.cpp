#include "parser.h"

#include <cstddef>
#include <cstdio>
#include <regex>
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
  bool prevSpace = false;

  for (char c : type) {
    if (c == ' ') {
      prevSpace = true;
    } else {
      if ((c == '*' || c == '&') && prevSpace && !result.empty()) {
        result.pop_back(); // remove space before * or &
      }
      result += c;
      prevSpace = false;
    }
  }

  // Remove 'const' for looser comparison
  result = std::regex_replace(result, std::regex("\\bconst\\b"), "");

  // Remove all remaining whitespace
  result = std::regex_replace(result, std::regex("\\s+"), "");

  return result;
}

bool parseFunctionSignature(std::string_view input, Signature &output) {
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

  printf("User Input Signarture:\n\t %s\n\n", toString(output).c_str());

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
