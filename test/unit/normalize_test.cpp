// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Unit tests for type normalization.

#include "coogle/arena.h"
#include "coogle/parser.h"
#include <gtest/gtest.h>

using namespace coogle;

// Test basic type normalization
TEST(NormalizeTypeTest, BasicTypes) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "int"), "int");
  EXPECT_EQ(normalizeType(Arena, "void"), "void");
  EXPECT_EQ(normalizeType(Arena, "char"), "char");
  EXPECT_EQ(normalizeType(Arena, "double"), "double");
  EXPECT_EQ(normalizeType(Arena, "float"), "float");
}

// Test whitespace removal
TEST(NormalizeTypeTest, WhitespaceRemoval) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "int "), "int");
  EXPECT_EQ(normalizeType(Arena, " int"), "int");
  EXPECT_EQ(normalizeType(Arena, "  int  "), "int");
  EXPECT_EQ(normalizeType(Arena, "char *"), "char*");
  EXPECT_EQ(normalizeType(Arena, "char  *"), "char*");
  EXPECT_EQ(normalizeType(Arena, "unsigned   int"), "unsignedint");
}

// Test const qualifier removal
TEST(NormalizeTypeTest, ConstRemoval) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "const int"), "int");
  EXPECT_EQ(normalizeType(Arena, "int const"), "int");
  EXPECT_EQ(normalizeType(Arena, "const char *"), "char*");
  EXPECT_EQ(normalizeType(Arena, "char * const"), "char*");
  EXPECT_EQ(normalizeType(Arena, "const char * const"), "char*");
}

// Test struct/class/union qualifier removal
TEST(NormalizeTypeTest, QualifierRemoval) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "struct Node"), "Node");
  EXPECT_EQ(normalizeType(Arena, "class MyClass"), "MyClass");
  EXPECT_EQ(normalizeType(Arena, "union Data"), "Data");
  EXPECT_EQ(normalizeType(Arena, "const struct Node *"), "Node*");
}

// Test pointer types
TEST(NormalizeTypeTest, PointerTypes) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "int *"), "int*");
  EXPECT_EQ(normalizeType(Arena, "char *"), "char*");
  EXPECT_EQ(normalizeType(Arena, "void *"), "void*");
  EXPECT_EQ(normalizeType(Arena, "int**"), "int**");
  EXPECT_EQ(normalizeType(Arena, "char * *"), "char**");
  EXPECT_EQ(normalizeType(Arena, "int * * *"), "int***");
}

// Test reference types
TEST(NormalizeTypeTest, ReferenceTypes) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "int &"), "int&");
  EXPECT_EQ(normalizeType(Arena, "const int &"), "int&");
  EXPECT_EQ(normalizeType(Arena, "int&&"), "int&&");
  EXPECT_EQ(normalizeType(Arena, "const int&&"), "int&&");
}

// Test const edge cases
TEST(NormalizeTypeTest, ConstEdgeCases) {
  StringArena Arena;
  // "const" as part of a word should not be removed
  EXPECT_EQ(normalizeType(Arena, "constant"), "constant");
  EXPECT_EQ(normalizeType(Arena, "myconst"), "myconst");

  // Only standalone "const" should be removed
  EXPECT_EQ(normalizeType(Arena, "const"), "");
  EXPECT_EQ(normalizeType(Arena, "const const"), "");
}

// Test std::string normalization
TEST(NormalizeTypeTest, StdString) {
  StringArena Arena;
  EXPECT_EQ(normalizeType(Arena, "std::string"), "std::string");
  EXPECT_EQ(normalizeType(Arena, "const std::string &"), "std::string&");
  EXPECT_EQ(normalizeType(Arena, "std::basic_string<char>"), "std::string");
  EXPECT_EQ(normalizeType(Arena, "std::basic_string<char, std::char_traits<char>, "
                                  "std::allocator<char>>"),
            "std::string");
}
