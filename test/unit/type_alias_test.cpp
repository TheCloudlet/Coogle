// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Unit tests for type alias handling in signature parsing and matching.
//
// CRITICAL DISTINCTION:
// =====================
// These unit tests do NOT test real type alias resolution from C++ code.
// They ONLY test that the parser can handle type alias NAMES in signature strings.
//
// What these tests verify:
//   - parseFunctionSignature("MyInt()") correctly parses the STRING "MyInt()"
//   - The parser preserves the name "MyInt" without corrupting it
//   - Normalization works correctly on alias names (removing const, etc.)
//   - Matching works when comparing signatures with alias names
//
// What these tests DO NOT verify:
//   - Whether libclang correctly resolves C++ typedef/using declarations
//   - Whether "typedef int MyInt; MyInt foo();" is found when searching
//   - The actual end-to-end behavior with real C++ source files
//
// REAL type alias handling:
// =========================
// Real type alias resolution happens in main.cpp via libclang:
//   CXType RetType = clang_getCursorResultType(Cursor);  // libclang does resolution
//   CXString RetSpelling = clang_getTypeSpelling(RetType);
//
// libclang is responsible for:
//   - Parsing actual C++ code
//   - Resolving typedef/using declarations
//   - Reporting type names back to our parser
//
// Integration testing (manual):
//   - test/inputs/type_alias_test.cpp contains real C++ type aliases
//   - Running: ./coogle test/inputs/type_alias_test.cpp "MyInt()"
//   - This tests the FULL pipeline: libclang → our parser → matching
//
// Behavior in production (main.cpp):
//   - User-defined type aliases (using/typedef) are preserved by libclang
//   - System type aliases (std::int32_t, etc.) may be canonicalized to underlying types
//   - This is expected libclang behavior, not a parser limitation

#include "coogle/arena.h"
#include "coogle/parser.h"
#include <gtest/gtest.h>

using namespace coogle;

// Test that type aliases in signatures are preserved as-is
TEST(TypeAliasTest, PreserveAliasNames) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "MyInt()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "MyInt");
  EXPECT_EQ(Sig->ArgTypes.size(), 0);

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "void(Integer)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "Integer");
}

// Test that type aliases normalize like regular types
TEST(TypeAliasTest, AliasNormalization) {
  StringArena Arena;

  // Type aliases should preserve their names during normalization
  EXPECT_EQ(normalizeType(Arena, "MyInt"), "MyInt");
  EXPECT_EQ(normalizeType(Arena, "const MyInt"), "MyInt");
  EXPECT_EQ(normalizeType(Arena, "MyInt *"), "MyInt*");
  EXPECT_EQ(normalizeType(Arena, "const MyInt *"), "MyInt*");
}

// Test matching with type aliases - exact alias name match
TEST(TypeAliasTest, ExactAliasMatch) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "MyInt()");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "MyInt()");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test that type aliases don't match underlying types
// (This is expected behavior - aliases are treated as distinct types)
TEST(TypeAliasTest, AliasDoesNotMatchUnderlyingType) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "MyInt()");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "int()");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void(Integer)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void(int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));
}

// Test pointer type aliases
TEST(TypeAliasTest, PointerAliases) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "StringPtr()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "StringPtr");

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "void(ConstCharPtr)");
  ASSERT_TRUE(Sig.has_value());
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "ConstCharPtr");
}

// Test function pointer type aliases
TEST(TypeAliasTest, FunctionPointerAliases) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "void(Callback)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "Callback");
}

// Test std library type aliases (like size_t, int32_t)
// Note: These tests verify the *parsing* of type aliases.
// When used with libclang in production, many standard library type aliases
// (like std::int32_t) may be canonicalized to their underlying types (int).
// This is expected libclang behavior and not a limitation of the parser.
TEST(TypeAliasTest, StdLibraryAliases) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "void(std::size_t)");
  ASSERT_TRUE(Sig.has_value());
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "std::size_t");

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "std::int32_t()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "std::int32_t");
}

// Test const with type aliases
TEST(TypeAliasTest, ConstWithAliases) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "const MyInt()");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "MyInt()");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void(const Integer)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void(Integer)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test wildcard matching with type aliases
TEST(TypeAliasTest, WildcardWithAliases) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "void(*)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "void(MyInt)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void(Integer, *)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void(Integer, MyInt)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test complex type aliases with templates
TEST(TypeAliasTest, TemplateAliases) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "std::vector<MyInt>(const std::vector<Integer> &)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "std::vector<MyInt>");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "const std::vector<Integer> &");
}

// Test toString with type aliases
TEST(TypeAliasTest, ToStringWithAliases) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "MyInt(Integer, ConstCharPtr)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "MyInt(Integer, ConstCharPtr)");
}
