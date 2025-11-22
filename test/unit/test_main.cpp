// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Main test runner for all unit tests.

#include <gtest/gtest.h>

int main(int Argc, char **Argv) {
  ::testing::InitGoogleTest(&Argc, Argv);
  return RUN_ALL_TESTS();
}
