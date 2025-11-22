#include "coogle/parser.h"
#include <gtest/gtest.h>

// Test basic type normalization
TEST(NormalizeTypeTest, BasicTypes) {
  EXPECT_EQ(normalizeType("int"), "int");
  EXPECT_EQ(normalizeType("void"), "void");
  EXPECT_EQ(normalizeType("char"), "char");
  EXPECT_EQ(normalizeType("double"), "double");
  EXPECT_EQ(normalizeType("float"), "float");
}

// Test whitespace removal
TEST(NormalizeTypeTest, WhitespaceRemoval) {
  EXPECT_EQ(normalizeType("int "), "int");
  EXPECT_EQ(normalizeType(" int"), "int");
  EXPECT_EQ(normalizeType("  int  "), "int");
  EXPECT_EQ(normalizeType("char *"), "char*");
  EXPECT_EQ(normalizeType("char  *"), "char*");
  EXPECT_EQ(normalizeType("unsigned   int"), "unsignedint");
}

// Test const qualifier removal
TEST(NormalizeTypeTest, ConstRemoval) {
  EXPECT_EQ(normalizeType("const int"), "int");
  EXPECT_EQ(normalizeType("int const"), "int");
  EXPECT_EQ(normalizeType("const char *"), "char*");
  EXPECT_EQ(normalizeType("char * const"), "char*");
  EXPECT_EQ(normalizeType("const char * const"), "char*");
}

// Test pointer types
TEST(NormalizeTypeTest, PointerTypes) {
  EXPECT_EQ(normalizeType("int *"), "int*");
  EXPECT_EQ(normalizeType("char *"), "char*");
  EXPECT_EQ(normalizeType("void *"), "void*");
  EXPECT_EQ(normalizeType("int**"), "int**");
  EXPECT_EQ(normalizeType("char * *"), "char**");
}

// Test const edge cases
TEST(NormalizeTypeTest, ConstEdgeCases) {
  // "const" as part of a word should not be removed
  EXPECT_EQ(normalizeType("constant"), "constant");
  EXPECT_EQ(normalizeType("myconst"), "myconst");

  // Only standalone "const" should be removed
  EXPECT_EQ(normalizeType("const"), "");
  EXPECT_EQ(normalizeType("const const"), "");
}

// Test std::string normalization
TEST(NormalizeTypeTest, StdString) {
  EXPECT_EQ(normalizeType("std::string"), "std::string");
  EXPECT_EQ(normalizeType("const std::string &"), "std::string&");
  EXPECT_EQ(normalizeType("std::basic_string<char>"), "std::string");
  EXPECT_EQ(normalizeType("std::basic_string<char, std::char_traits<char>, "
                          "std::allocator<char>>"),
            "std::string");
}

// Test signature parsing - basic cases
TEST(ParseSignatureTest, BasicSignatures) {
  auto Sig = parseFunctionSignature("void()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  EXPECT_EQ(Sig->ArgType.size(), 0);

  Sig = parseFunctionSignature("int()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  EXPECT_EQ(Sig->ArgType.size(), 0);
}

// Test signature parsing - single argument
TEST(ParseSignatureTest, SingleArgument) {
  auto Sig = parseFunctionSignature("int(int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgType.size(), 1);
  EXPECT_EQ(Sig->ArgType[0], "int");

  Sig = parseFunctionSignature("void(char *)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgType.size(), 1);
  EXPECT_EQ(Sig->ArgType[0], "char *");
}

// Test signature parsing - multiple arguments
TEST(ParseSignatureTest, MultipleArguments) {
  auto Sig = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgType.size(), 2);
  EXPECT_EQ(Sig->ArgType[0], "int");
  EXPECT_EQ(Sig->ArgType[1], "int");

  Sig = parseFunctionSignature("void(int, char *, double)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgType.size(), 3);
  EXPECT_EQ(Sig->ArgType[0], "int");
  EXPECT_EQ(Sig->ArgType[1], "char *");
  EXPECT_EQ(Sig->ArgType[2], "double");
}

// Test signature parsing - with whitespace
TEST(ParseSignatureTest, WithWhitespace) {
  auto Sig = parseFunctionSignature("int ( int , int )");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgType.size(), 2);
  EXPECT_EQ(Sig->ArgType[0], "int");
  EXPECT_EQ(Sig->ArgType[1], "int");
}

// Test signature parsing - invalid inputs
TEST(ParseSignatureTest, InvalidInputs) {
  EXPECT_FALSE(parseFunctionSignature("invalid").has_value());
  EXPECT_FALSE(parseFunctionSignature("no_parens").has_value());
  EXPECT_FALSE(parseFunctionSignature("int(").has_value());
  EXPECT_FALSE(parseFunctionSignature("int)").has_value());
  EXPECT_FALSE(parseFunctionSignature(")(").has_value());
}

// Test signature matching - exact matches
TEST(SignatureMatchTest, ExactMatches) {
  auto A = parseFunctionSignature("int(int, int)");
  auto B = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  A = parseFunctionSignature("void()");
  B = parseFunctionSignature("void()");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  A = parseFunctionSignature("char *(int, char *)");
  B = parseFunctionSignature("char *(int, char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - with const
TEST(SignatureMatchTest, WithConst) {
  auto A = parseFunctionSignature("int(const int)");
  auto B = parseFunctionSignature("int(int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  A = parseFunctionSignature("const int(int)");
  B = parseFunctionSignature("int(int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  A = parseFunctionSignature("void(const char *)");
  B = parseFunctionSignature("void(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - with whitespace
TEST(SignatureMatchTest, WithWhitespace) {
  auto A = parseFunctionSignature("int(int,int)");
  auto B = parseFunctionSignature("int( int , int )");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  A = parseFunctionSignature("char*(int)");
  B = parseFunctionSignature("char * ( int )");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test signature matching - mismatches
TEST(SignatureMatchTest, Mismatches) {
  // Different return types
  auto A = parseFunctionSignature("int(int)");
  auto B = parseFunctionSignature("void(int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Different argument counts
  A = parseFunctionSignature("int(int)");
  B = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Different argument types
  A = parseFunctionSignature("int(int)");
  B = parseFunctionSignature("int(char)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Pointer vs non-pointer
  A = parseFunctionSignature("int(int)");
  B = parseFunctionSignature("int(int *)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));
}

// Test signature matching - with wildcards
TEST(SignatureMatchTest, WildcardMatching) {
  // Wildcard in first position
  auto A = parseFunctionSignature("int(*, int)");
  auto B = parseFunctionSignature("int(char, int)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Wildcard in last position
  A = parseFunctionSignature("void(int, *)");
  B = parseFunctionSignature("void(int, const char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Multiple wildcards
  A = parseFunctionSignature("void(*, *)");
  B = parseFunctionSignature("void(int, double)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // Mismatch return type with wildcard
  A = parseFunctionSignature("int(*)");
  B = parseFunctionSignature("void(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));

  // Mismatch argument count with wildcard
  A = parseFunctionSignature("int(*)");
  B = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(A && B);
  EXPECT_FALSE(isSignatureMatch(*A, *B));
}

// Test toString
TEST(ToStringTest, BasicConversion) {
  auto Sig = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "int(int, int)");

  Sig = parseFunctionSignature("void()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "void()");

  Sig = parseFunctionSignature("char *(int, char *, double)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "char *(int, char *, double)");
}

// Test complex real-world signatures
TEST(SignatureMatchTest, RealWorldCases) {
  // FILE * fopen(const char *, const char *)
  auto A = parseFunctionSignature("FILE *(const char *, const char *)");
  auto B = parseFunctionSignature("FILE *(char *, char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // void * malloc(size_t)
  A = parseFunctionSignature("void *(size_t)");
  B = parseFunctionSignature("void *(size_t)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // int printf(const char *, ...)  - Note: ... not supported yet
  // This will be parsed as just the first arg
  A = parseFunctionSignature("int(const char *)");
  B = parseFunctionSignature("int(char *)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));

  // std::string greet(const std::string &)
  A = parseFunctionSignature("std::string(const std::string &)");
  B = parseFunctionSignature(
      "std::basic_string<char, std::char_traits<char>, "
      "std::allocator<char>>(const std::basic_string<char> &)");
  ASSERT_TRUE(A && B);
  EXPECT_TRUE(isSignatureMatch(*A, *B));
}

// Test wildcard matching against the functions in example.c
TEST(WildcardIntegrationTest, ExampleFileFunctions) {
  // int add(int, int)
  auto WildcardQuery = parseFunctionSignature("int(*, *)");
  auto ActualFunc = parseFunctionSignature("int(int, int)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // void increment(int *)
  WildcardQuery = parseFunctionSignature("void(*)");
  ActualFunc = parseFunctionSignature("void(int *)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // void process(void *, int)
  WildcardQuery = parseFunctionSignature("void(void *, *)");
  ActualFunc = parseFunctionSignature("void(void *, int)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));

  // const char *getMessage()
  WildcardQuery = parseFunctionSignature("const char *()");
  ActualFunc = parseFunctionSignature("const char *()");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery,
                               *ActualFunc)); // No wildcard, just a sanity check

  // bool processData(const std::string &, void *, size_t)
  WildcardQuery = parseFunctionSignature("bool(const std::string &, *, *)");
  ActualFunc = parseFunctionSignature("bool(const std::string &, void *, size_t)");
  ASSERT_TRUE(WildcardQuery && ActualFunc);
  EXPECT_TRUE(isSignatureMatch(*WildcardQuery, *ActualFunc));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
