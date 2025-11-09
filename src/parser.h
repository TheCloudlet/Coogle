#pragma once
#include <string>
#include <string_view>
#include <vector>

struct Signature {
  std::string RetType;
  std::vector<std::string> ArgType;
};

bool parseFunctionSignature(std::string_view input, Signature &output);
std::string toString(const Signature &sig);
std::string normalizeType(std::string_view type);
bool isSignatureMatch(const Signature &userSig, const Signature &actualSig);
