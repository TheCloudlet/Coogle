#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <vector>

struct Signature {
  std::string RetType;
  std::vector<std::string> ArgType;
};

std::optional<Signature> parseFunctionSignature(std::string_view input);
std::string toString(const Signature &sig);
std::string normalizeType(std::string_view type);
bool isSignatureMatch(const Signature &userSig, const Signature &actualSig);
