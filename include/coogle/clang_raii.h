// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// RAII wrappers for libclang resources to ensure proper cleanup.

#pragma once
#include <clang-c/Index.h>

// RAII wrapper for CXIndex to ensure proper resource cleanup.
class CXIndexRAII {
  CXIndex Index_;

public:
  CXIndexRAII() : Index_(clang_createIndex(0, 0)) {}

  ~CXIndexRAII() {
    if (Index_) {
      clang_disposeIndex(Index_);
    }
  }

  // Delete copy constructor and assignment operator
  CXIndexRAII(const CXIndexRAII &) = delete;
  CXIndexRAII &operator=(const CXIndexRAII &) = delete;

  // Allow move semantics
  CXIndexRAII(CXIndexRAII &&Other) noexcept : Index_(Other.Index_) {
    Other.Index_ = nullptr;
  }

  CXIndexRAII &operator=(CXIndexRAII &&Other) noexcept {
    if (this != &Other) {
      if (Index_) {
        clang_disposeIndex(Index_);
      }
      Index_ = Other.Index_;
      Other.Index_ = nullptr;
    }
    return *this;
  }

  // Implicit conversion to CXIndex for use with libclang API
  operator CXIndex() const { return Index_; }

  // Explicit validity check
  bool isValid() const { return Index_ != nullptr; }

  // Get raw pointer (use sparingly)
  CXIndex get() const { return Index_; }
};

// RAII wrapper for CXTranslationUnit to ensure proper resource cleanup.
class CXTranslationUnitRAII {
  CXTranslationUnit TU_;

public:
  // Constructor takes ownership of an existing CXTranslationUnit
  explicit CXTranslationUnitRAII(CXTranslationUnit TU) : TU_(TU) {}

  ~CXTranslationUnitRAII() {
    if (TU_) {
      clang_disposeTranslationUnit(TU_);
    }
  }

  // Delete copy constructor and assignment operator
  CXTranslationUnitRAII(const CXTranslationUnitRAII &) = delete;
  CXTranslationUnitRAII &operator=(const CXTranslationUnitRAII &) = delete;

  // Allow move semantics
  CXTranslationUnitRAII(CXTranslationUnitRAII &&Other) noexcept
      : TU_(Other.TU_) {
    Other.TU_ = nullptr;
  }

  CXTranslationUnitRAII &operator=(CXTranslationUnitRAII &&Other) noexcept {
    if (this != &Other) {
      if (TU_) {
        clang_disposeTranslationUnit(TU_);
      }
      TU_ = Other.TU_;
      Other.TU_ = nullptr;
    }
    return *this;
  }

  // Implicit conversion to CXTranslationUnit for use with libclang API
  operator CXTranslationUnit() const { return TU_; }

  // Explicit validity check
  bool isValid() const { return TU_ != nullptr; }

  // Get raw pointer (use sparingly)
  CXTranslationUnit get() const { return TU_; }
};

// RAII wrapper for CXString to ensure proper resource cleanup.
class CXStringRAII {
  CXString Str_;

public:
  explicit CXStringRAII(CXString Str) : Str_(Str) {}

  ~CXStringRAII() { clang_disposeString(Str_); }

  // Delete copy constructor and assignment operator
  CXStringRAII(const CXStringRAII &) = delete;
  CXStringRAII &operator=(const CXStringRAII &) = delete;

  // Allow move semantics
  CXStringRAII(CXStringRAII &&Other) noexcept : Str_(Other.Str_) {
    Other.Str_ = {nullptr, 0};
  }

  CXStringRAII &operator=(CXStringRAII &&Other) noexcept {
    if (this != &Other) {
      clang_disposeString(Str_);
      Str_ = Other.Str_;
      Other.Str_ = {nullptr, 0};
    }
    return *this;
  }

  // Get C string (can return nullptr)
  const char *c_str() const { return clang_getCString(Str_); }

  // Implicit conversion to CXString for use with libclang API
  operator CXString() const { return Str_; }
};
