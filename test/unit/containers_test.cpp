// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Unit tests for STL container signature matching.

#include "coogle/arena.h"
#include "coogle/parser.h"
#include <gtest/gtest.h>

using namespace coogle;

// Test std::vector normalization and matching
TEST(ContainersTest, StdVector) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::vector<int>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::vector<int>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

TEST(ContainersTest, StdVectorWithConst) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::vector<int>(const std::vector<int> &)");
  auto Sig2 = parseFunctionSignature(Storage2, "std::vector<int>(std::vector<int> &)");
  ASSERT_TRUE(Sig1 && Sig2);
  // Should match because const is normalized away
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

TEST(ContainersTest, StdVectorNested) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::vector<std::vector<int>>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::vector<std::vector<int>>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::map
TEST(ContainersTest, StdMap) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::map<int, std::string>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::map<int, std::string>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

TEST(ContainersTest, StdMapWithSpaces) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::map<int, std::string>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::map< int , std::string >()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::pair
TEST(ContainersTest, StdPair) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::pair<int, double>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::pair<int, double>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::set
TEST(ContainersTest, StdSet) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::set<std::string>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::set<std::string>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::unordered_map
TEST(ContainersTest, StdUnorderedMap) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1,
      "std::unordered_map<std::string, int>()");
  auto Sig2 = parseFunctionSignature(Storage2,
      "std::unordered_map<std::string, int>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::shared_ptr
TEST(ContainersTest, StdSharedPtr) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::shared_ptr<MyClass>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::shared_ptr<MyClass>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::unique_ptr
TEST(ContainersTest, StdUniquePtr) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::unique_ptr<int>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::unique_ptr<int>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::optional (C++17)
TEST(ContainersTest, StdOptional) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::optional<int>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::optional<int>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::array
TEST(ContainersTest, StdArray) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::array<int, 5>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::array<int, 5>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test std::tuple
TEST(ContainersTest, StdTuple) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "std::tuple<int, double, std::string>()");
  auto Sig2 = parseFunctionSignature(Storage2, "std::tuple<int, double, std::string>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test complex nested containers
TEST(ContainersTest, ComplexNested) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1,
      "std::map<std::string, std::vector<std::shared_ptr<int>>>()");
  auto Sig2 = parseFunctionSignature(Storage2,
      "std::map<std::string, std::vector<std::shared_ptr<int>>>()");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test container with function taking container
TEST(ContainersTest, ContainerAsArgument) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1,
      "void(const std::vector<int> &, std::map<std::string, double> *)");
  auto Sig2 = parseFunctionSignature(Storage2,
      "void(std::vector<int> &, std::map<std::string, double> *)");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}

// Test wildcard with containers
TEST(ContainersTest, WildcardWithContainers) {
  SignatureStorage Storage1, Storage2;

  auto Sig1 = parseFunctionSignature(Storage1, "void(std::vector<int>, *)");
  auto Sig2 = parseFunctionSignature(Storage2, "void(std::vector<int>, std::string)");
  ASSERT_TRUE(Sig1 && Sig2);
  EXPECT_TRUE(isSignatureMatch(*Sig1, *Sig2));
}
