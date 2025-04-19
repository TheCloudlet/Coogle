#pragma once
#include <string>
#include <string_view>
#include <vector>

struct Signature {
  std::string funcName;
  std::string retType;
  std::vector<std::string> argType;
};

bool parseFunctionSignature(std::string_view input, Signature &output);
std::string normalizeType(std::string_view type);
bool isSignatureMatch(const Signature &userSig, const Signature &actualSig);
