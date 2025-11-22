// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Core API for parsing and matching C++ function signatures with
// zero-allocation semantics.

#pragma once

#include "arena.h"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace coogle {

// Function signature with zero-allocation string_view references.
// All string_views must point into a StringArena that outlives this struct.
//
// Pre-normalized types are stored for O(1) matching without repeated
// normalization.
struct Signature {
  std::string_view RetType;            // Original return type
  std::string_view RetTypeNorm;        // Normalized return type
  span<std::string_view> ArgTypes;     // Original argument types
  span<std::string_view> ArgTypesNorm; // Normalized argument types
} __attribute__((packed));

// Helper class to manage signature storage with arena-backed strings.
// Provides convenient methods to build signatures incrementally.
class SignatureStorage {
  StringArena Strings_;
  std::vector<std::string_view> ArgBuffer_;
  std::vector<std::string_view> ArgNormBuffer_;

public:
  // Interns a string into the arena.
  std::string_view internString(std::string_view Str) {
    return Strings_.intern(Str);
  }

  // Reserves space for a specific number of arguments.
  void reserveArgs(size_t Count) {
    ArgBuffer_.clear();
    ArgBuffer_.reserve(Count);
    ArgNormBuffer_.clear();
    ArgNormBuffer_.reserve(Count);
  }

  // Adds an argument (original and normalized versions).
  void addArg(std::string_view Arg, std::string_view ArgNorm) {
    ArgBuffer_.push_back(Arg);
    ArgNormBuffer_.push_back(ArgNorm);
  }

  // Gets span of original arguments.
  span<std::string_view> getArgs() {
    return span<std::string_view>(ArgBuffer_.data(), ArgBuffer_.size());
  }

  // Gets span of normalized arguments.
  span<std::string_view> getArgsNorm() {
    return span<std::string_view>(ArgNormBuffer_.data(), ArgNormBuffer_.size());
  }

  // Gets access to the underlying arena for custom operations.
  StringArena &arena() { return Strings_; }
};

// Parses a function signature string into a Signature struct.
// Returns nullopt if the signature is malformed.
//
// The signature must outlive the SignatureStorage.
// All strings are normalized during parsing for efficient matching.
std::optional<Signature> parseFunctionSignature(SignatureStorage &Storage,
                                                std::string_view Input);

// Converts a signature to a human-readable string (for display).
// This allocates a new string for output purposes.
std::string toString(const Signature &Sig);

// Normalizes a type string by removing whitespace and keywords.
// The normalized string is written into the arena.
// This is the core transformation for type matching.
std::string_view normalizeType(StringArena &Arena, std::string_view Type);

// Checks if two signatures match.
// Uses pre-normalized types for O(1) comparison.
// Supports wildcard matching with "*" in argument types.
bool isSignatureMatch(const Signature &UserSig, const Signature &ActualSig);

} // namespace coogle
