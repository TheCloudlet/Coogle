// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Unit tests for signature parsing and toString.

#include "coogle/arena.h"
#include "coogle/parser.h"
#include <gtest/gtest.h>

using namespace coogle;

// Test signature parsing - basic cases
TEST(ParseSignatureTest, BasicSignatures) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "void()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  EXPECT_EQ(Sig->ArgTypes.size(), 0);

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "int()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  EXPECT_EQ(Sig->ArgTypes.size(), 0);
}

// Test signature parsing - single argument
TEST(ParseSignatureTest, SingleArgument) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "int(int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "int");

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "void(char *)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "char *");
}

// Test signature parsing - multiple arguments
TEST(ParseSignatureTest, MultipleArguments) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "int(int, int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgTypes.size(), 2);
  EXPECT_EQ(Sig->ArgTypes[0], "int");
  EXPECT_EQ(Sig->ArgTypes[1], "int");

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "void(int, char *, double)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgTypes.size(), 3);
  EXPECT_EQ(Sig->ArgTypes[0], "int");
  EXPECT_EQ(Sig->ArgTypes[1], "char *");
  EXPECT_EQ(Sig->ArgTypes[2], "double");
}

// Test signature parsing - with whitespace
TEST(ParseSignatureTest, WithWhitespace) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "int ( int , int )");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgTypes.size(), 2);
  EXPECT_EQ(Sig->ArgTypes[0], "int");
  EXPECT_EQ(Sig->ArgTypes[1], "int");
}

// Test signature parsing - complex templates
TEST(ParseSignatureTest, ComplexTemplates) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage,
      "std::vector<int>(const std::vector<int> &, size_t)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "std::vector<int>");
  ASSERT_EQ(Sig->ArgTypes.size(), 2);
  EXPECT_EQ(Sig->ArgTypes[0], "const std::vector<int> &");
  EXPECT_EQ(Sig->ArgTypes[1], "size_t");
}

// Test signature parsing - invalid inputs
TEST(ParseSignatureTest, InvalidInputs) {
  SignatureStorage Storage;
  EXPECT_FALSE(parseFunctionSignature(Storage, "invalid").has_value());

  SignatureStorage Storage2;
  EXPECT_FALSE(parseFunctionSignature(Storage2, "no_parens").has_value());

  SignatureStorage Storage3;
  EXPECT_FALSE(parseFunctionSignature(Storage3, "int(").has_value());

  SignatureStorage Storage4;
  EXPECT_FALSE(parseFunctionSignature(Storage4, "int)").has_value());

  SignatureStorage Storage5;
  EXPECT_FALSE(parseFunctionSignature(Storage5, ")(").has_value());
}

// Test toString
TEST(ToStringTest, BasicConversion) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "int(int, int)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "int(int, int)");

  SignatureStorage Storage2;
  Sig = parseFunctionSignature(Storage2, "void()");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "void()");

  SignatureStorage Storage3;
  Sig = parseFunctionSignature(Storage3, "char *(int, char *, double)");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(toString(*Sig), "char *(int, char *, double)");
}

// Test function pointers
TEST(ParseSignatureTest, FunctionPointers) {
  SignatureStorage Storage;
  auto Sig = parseFunctionSignature(Storage, "void(void (*)(int))");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "void");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "void (*)(int)");
}

// Test function pointers as return types
TEST(ParseSignatureTest, FunctionPointerReturn) {
  SignatureStorage Storage;
  // This is actually parsed as: return type "int", one argument "(*)(void)"
  // Function pointers as return types need special syntax: int(*)() returns pointer-to-function
  auto Sig = parseFunctionSignature(Storage, "int((*)(void))");
  ASSERT_TRUE(Sig.has_value());
  EXPECT_EQ(Sig->RetType, "int");
  ASSERT_EQ(Sig->ArgTypes.size(), 1);
  EXPECT_EQ(Sig->ArgTypes[0], "(*)(void)");
}
