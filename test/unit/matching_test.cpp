// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Unit tests for signature matching and wildcard support.

#include "coogle/arena.h"
#include "coogle/parser.h"
#include <gtest/gtest.h>

using namespace coogle;

// Test signature matching - exact matches
TEST(SignatureMatchTest, ExactMatches) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "int(int, int)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void()");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void()");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageE;
  A = parseFunctionSignature(StorageE, "char *(int, char *)");
  SignatureStorage StorageF;
  B = parseFunctionSignature(StorageF, "char *(int, char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - with const
TEST(SignatureMatchTest, WithConst) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "int(const int)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "int(int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "const int(int)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "int(int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageE;
  A = parseFunctionSignature(StorageE, "void(const char *)");
  SignatureStorage StorageF;
  B = parseFunctionSignature(StorageF, "void(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - with whitespace
TEST(SignatureMatchTest, WithWhitespace) {
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "int(int,int)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "int( int , int )");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "char*(int)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "char * ( int )");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - mismatches
TEST(SignatureMatchTest, Mismatches) {
  // Different return types
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "int(int)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "void(int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Different argument counts
  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "int(int)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Different argument types
  SignatureStorage StorageE;
  A = parseFunctionSignature(StorageE, "int(int)");
  SignatureStorage StorageF;
  B = parseFunctionSignature(StorageF, "int(char)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Pointer vs non-pointer
  SignatureStorage StorageG;
  A = parseFunctionSignature(StorageG, "int(int)");
  SignatureStorage StorageH;
  B = parseFunctionSignature(StorageH, "int(int *)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));
}

// Test signature matching - with wildcards
TEST(SignatureMatchTest, WildcardMatching) {
  // Wildcard in first position
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "int(*, int)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "int(char, int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Wildcard in last position
  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void(int, *)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void(int, const char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Multiple wildcards
  SignatureStorage StorageE;
  A = parseFunctionSignature(StorageE, "void(*, *)");
  SignatureStorage StorageF;
  B = parseFunctionSignature(StorageF, "void(int, double)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Mismatch return type with wildcard
  SignatureStorage StorageG;
  A = parseFunctionSignature(StorageG, "int(*)");
  SignatureStorage StorageH;
  B = parseFunctionSignature(StorageH, "void(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Mismatch argument count with wildcard
  SignatureStorage StorageI;
  A = parseFunctionSignature(StorageI, "int(*)");
  SignatureStorage StorageJ;
  B = parseFunctionSignature(StorageJ, "int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));
}

// Test complex real-world signatures
TEST(SignatureMatchTest, RealWorldCases) {
  // FILE * fopen(const char *, const char *)
  SignatureStorage StorageA;
  auto A = parseFunctionSignature(StorageA, "FILE *(const char *, const char *)");
  SignatureStorage StorageB;
  auto B = parseFunctionSignature(StorageB, "FILE *(char *, char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // void * malloc(size_t)
  SignatureStorage StorageC;
  A = parseFunctionSignature(StorageC, "void *(size_t)");
  SignatureStorage StorageD;
  B = parseFunctionSignature(StorageD, "void *(size_t)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // int printf(const char *, ...)  - Note: ... not supported yet
  // This will be parsed as just the first arg
  SignatureStorage StorageE;
  A = parseFunctionSignature(StorageE, "int(const char *)");
  SignatureStorage StorageF;
  B = parseFunctionSignature(StorageF, "int(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // std::string greet(const std::string &)
  SignatureStorage StorageG;
  A = parseFunctionSignature(StorageG, "std::string(const std::string &)");
  SignatureStorage StorageH;
  B = parseFunctionSignature(StorageH,
      "std::basic_string<char, std::char_traits<char>, "
      "std::allocator<char>>(const std::basic_string<char> &)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test wildcard matching against example functions
TEST(WildcardIntegrationTest, ExampleFileFunctions) {
  // int add(int, int)
  SignatureStorage StorageA;
  auto WildcardQuery = parseFunctionSignature(StorageA, "int(*, *)");
  SignatureStorage StorageB;
  auto ActualFunc = parseFunctionSignature(StorageB, "int(int, int)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // void increment(int *)
  SignatureStorage StorageC;
  WildcardQuery = parseFunctionSignature(StorageC, "void(*)");
  SignatureStorage StorageD;
  ActualFunc = parseFunctionSignature(StorageD, "void(int *)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // void process(void *, int)
  SignatureStorage StorageE;
  WildcardQuery = parseFunctionSignature(StorageE, "void(void *, *)");
  SignatureStorage StorageF;
  ActualFunc = parseFunctionSignature(StorageF, "void(void *, int)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // const char *getMessage()
  SignatureStorage StorageG;
  WildcardQuery = parseFunctionSignature(StorageG, "const char *()");
  SignatureStorage StorageH;
  ActualFunc = parseFunctionSignature(StorageH, "const char *()");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // bool processData(const std::string &, void *, size_t)
  SignatureStorage StorageI;
  WildcardQuery = parseFunctionSignature(StorageI, "bool(const std::string &, *, *)");
  SignatureStorage StorageJ;
  ActualFunc = parseFunctionSignature(StorageJ, "bool(const std::string &, void *, size_t)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));
}
