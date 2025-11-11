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

// Test signature parsing - basic cases
TEST(ParseSignatureTest, BasicSignatures) {
  Signature Sig;

  EXPECT_TRUE(parseFunctionSignature("void()", Sig));
  EXPECT_EQ(Sig.RetType, "void");
  EXPECT_EQ(Sig.ArgType.size(), 0);

  Sig = {};
  EXPECT_TRUE(parseFunctionSignature("int()", Sig));
  EXPECT_EQ(Sig.RetType, "int");
  EXPECT_EQ(Sig.ArgType.size(), 0);
}

// Test signature parsing - single argument
TEST(ParseSignatureTest, SingleArgument) {
  Signature Sig;

  EXPECT_TRUE(parseFunctionSignature("int(int)", Sig));
  EXPECT_EQ(Sig.RetType, "int");
  ASSERT_EQ(Sig.ArgType.size(), 1);
  EXPECT_EQ(Sig.ArgType[0], "int");

  Sig = {};
  EXPECT_TRUE(parseFunctionSignature("void(char *)", Sig));
  EXPECT_EQ(Sig.RetType, "void");
  ASSERT_EQ(Sig.ArgType.size(), 1);
  EXPECT_EQ(Sig.ArgType[0], "char *");
}

// Test signature parsing - multiple arguments
TEST(ParseSignatureTest, MultipleArguments) {
  Signature Sig;

  EXPECT_TRUE(parseFunctionSignature("int(int, int)", Sig));
  EXPECT_EQ(Sig.RetType, "int");
  ASSERT_EQ(Sig.ArgType.size(), 2);
  EXPECT_EQ(Sig.ArgType[0], "int");
  EXPECT_EQ(Sig.ArgType[1], "int");

  Sig = {};
  EXPECT_TRUE(parseFunctionSignature("void(int, char *, double)", Sig));
  EXPECT_EQ(Sig.RetType, "void");
  ASSERT_EQ(Sig.ArgType.size(), 3);
  EXPECT_EQ(Sig.ArgType[0], "int");
  EXPECT_EQ(Sig.ArgType[1], "char *");
  EXPECT_EQ(Sig.ArgType[2], "double");
}

// Test signature parsing - with whitespace
TEST(ParseSignatureTest, WithWhitespace) {
  Signature Sig;

  EXPECT_TRUE(parseFunctionSignature("int ( int , int )", Sig));
  EXPECT_EQ(Sig.RetType, "int ");
  ASSERT_EQ(Sig.ArgType.size(), 2);
  EXPECT_EQ(Sig.ArgType[0], "int");
  EXPECT_EQ(Sig.ArgType[1], "int");
}

// Test signature parsing - invalid inputs
TEST(ParseSignatureTest, InvalidInputs) {
  Signature Sig;

  EXPECT_FALSE(parseFunctionSignature("invalid", Sig));
  EXPECT_FALSE(parseFunctionSignature("no_parens", Sig));
  EXPECT_FALSE(parseFunctionSignature("int(", Sig));
  EXPECT_FALSE(parseFunctionSignature("int)", Sig));
  EXPECT_FALSE(parseFunctionSignature(")(", Sig));
}

// Test signature matching - exact matches
TEST(SignatureMatchTest, ExactMatches) {
  Signature A, B;

  parseFunctionSignature("int(int, int)", A);
  parseFunctionSignature("int(int, int)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  parseFunctionSignature("void()", A);
  parseFunctionSignature("void()", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  parseFunctionSignature("char *(int, char *)", A);
  parseFunctionSignature("char *(int, char *)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));
}

// Test signature matching - with const
TEST(SignatureMatchTest, WithConst) {
  Signature A, B;

  parseFunctionSignature("int(const int)", A);
  parseFunctionSignature("int(int)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  parseFunctionSignature("const int(int)", A);
  parseFunctionSignature("int(int)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  parseFunctionSignature("void(const char *)", A);
  parseFunctionSignature("void(char *)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));
}

// Test signature matching - with whitespace
TEST(SignatureMatchTest, WithWhitespace) {
  Signature A, B;

  parseFunctionSignature("int(int,int)", A);
  parseFunctionSignature("int( int , int )", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  parseFunctionSignature("char*(int)", A);
  parseFunctionSignature("char * ( int )", B);
  EXPECT_TRUE(isSignatureMatch(A, B));
}

// Test signature matching - mismatches
TEST(SignatureMatchTest, Mismatches) {
  Signature A, B;

  // Different return types
  parseFunctionSignature("int(int)", A);
  parseFunctionSignature("void(int)", B);
  EXPECT_FALSE(isSignatureMatch(A, B));

  // Different argument counts
  parseFunctionSignature("int(int)", A);
  parseFunctionSignature("int(int, int)", B);
  EXPECT_FALSE(isSignatureMatch(A, B));

  // Different argument types
  parseFunctionSignature("int(int)", A);
  parseFunctionSignature("int(char)", B);
  EXPECT_FALSE(isSignatureMatch(A, B));

  // Pointer vs non-pointer
  parseFunctionSignature("int(int)", A);
  parseFunctionSignature("int(int *)", B);
  EXPECT_FALSE(isSignatureMatch(A, B));
}

// Test toString
TEST(ToStringTest, BasicConversion) {
  Signature Sig;

  parseFunctionSignature("int(int, int)", Sig);
  EXPECT_EQ(toString(Sig), "int(int, int)");

  Sig = {}; // Reset
  parseFunctionSignature("void()", Sig);
  EXPECT_EQ(toString(Sig), "void()");

  Sig = {}; // Reset
  parseFunctionSignature("char *(int, char *, double)", Sig);
  EXPECT_EQ(toString(Sig), "char *(int, char *, double)");
}

// Test complex real-world signatures
TEST(SignatureMatchTest, RealWorldCases) {
  Signature A, B;

  // FILE * fopen(const char *, const char *)
  parseFunctionSignature("FILE *(const char *, const char *)", A);
  parseFunctionSignature("FILE *(char *, char *)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  // void * malloc(size_t)
  parseFunctionSignature("void *(size_t)", A);
  parseFunctionSignature("void *(size_t)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));

  // int printf(const char *, ...)  - Note: ... not supported yet
  // This will be parsed as just the first arg
  parseFunctionSignature("int(const char *)", A);
  parseFunctionSignature("int(char *)", B);
  EXPECT_TRUE(isSignatureMatch(A, B));
}

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
