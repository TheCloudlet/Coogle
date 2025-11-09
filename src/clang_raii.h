#pragma once
#include <clang-c/Index.h>

/// RAII wrapper for CXIndex to ensure proper resource cleanup
class CXIndexRAII {
  CXIndex index_;

public:
  CXIndexRAII() : index_(clang_createIndex(0, 0)) {}

  ~CXIndexRAII() {
    if (index_) {
      clang_disposeIndex(index_);
    }
  }

  // Delete copy constructor and assignment operator
  CXIndexRAII(const CXIndexRAII &) = delete;
  CXIndexRAII &operator=(const CXIndexRAII &) = delete;

  // Allow move semantics
  CXIndexRAII(CXIndexRAII &&other) noexcept : index_(other.index_) {
    other.index_ = nullptr;
  }

  CXIndexRAII &operator=(CXIndexRAII &&other) noexcept {
    if (this != &other) {
      if (index_) {
        clang_disposeIndex(index_);
      }
      index_ = other.index_;
      other.index_ = nullptr;
    }
    return *this;
  }

  // Implicit conversion to CXIndex for use with libclang API
  operator CXIndex() const { return index_; }

  // Explicit validity check
  bool isValid() const { return index_ != nullptr; }

  // Get raw pointer (use sparingly)
  CXIndex get() const { return index_; }
};

/// RAII wrapper for CXTranslationUnit to ensure proper resource cleanup
class CXTranslationUnitRAII {
  CXTranslationUnit tu_;

public:
  // Constructor takes ownership of an existing CXTranslationUnit
  explicit CXTranslationUnitRAII(CXTranslationUnit tu) : tu_(tu) {}

  ~CXTranslationUnitRAII() {
    if (tu_) {
      clang_disposeTranslationUnit(tu_);
    }
  }

  // Delete copy constructor and assignment operator
  CXTranslationUnitRAII(const CXTranslationUnitRAII &) = delete;
  CXTranslationUnitRAII &operator=(const CXTranslationUnitRAII &) = delete;

  // Allow move semantics
  CXTranslationUnitRAII(CXTranslationUnitRAII &&other) noexcept
      : tu_(other.tu_) {
    other.tu_ = nullptr;
  }

  CXTranslationUnitRAII &operator=(CXTranslationUnitRAII &&other) noexcept {
    if (this != &other) {
      if (tu_) {
        clang_disposeTranslationUnit(tu_);
      }
      tu_ = other.tu_;
      other.tu_ = nullptr;
    }
    return *this;
  }

  // Implicit conversion to CXTranslationUnit for use with libclang API
  operator CXTranslationUnit() const { return tu_; }

  // Explicit validity check
  bool isValid() const { return tu_ != nullptr; }

  // Get raw pointer (use sparingly)
  CXTranslationUnit get() const { return tu_; }
};

/// RAII wrapper for CXString to ensure proper resource cleanup
class CXStringRAII {
  CXString str_;

public:
  explicit CXStringRAII(CXString str) : str_(str) {}

  ~CXStringRAII() { clang_disposeString(str_); }

  // Delete copy constructor and assignment operator
  CXStringRAII(const CXStringRAII &) = delete;
  CXStringRAII &operator=(const CXStringRAII &) = delete;

  // Allow move semantics
  CXStringRAII(CXStringRAII &&other) noexcept : str_(other.str_) {
    other.str_ = {nullptr, 0};
  }

  CXStringRAII &operator=(CXStringRAII &&other) noexcept {
    if (this != &other) {
      clang_disposeString(str_);
      str_ = other.str_;
      other.str_ = {nullptr, 0};
    }
    return *this;
  }

  // Get C string (can return nullptr)
  const char *c_str() const { return clang_getCString(str_); }

  // Implicit conversion to CXString for use with libclang API
  operator CXString() const { return str_; }
};
