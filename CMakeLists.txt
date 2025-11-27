cmake_minimum_required(VERSION 3.14)
project(Coogle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -Wall -Wextra -g")

# Generate compile_commands.json for IDE integration and tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 10.2.1  # Use a specific, recent version
  GIT_SHALLOW TRUE  # To speed up cloning
)
FetchContent_MakeAvailable(fmt)

find_package(GTest CONFIG)

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config REQUIRED)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
                OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
                OUTPUT_VARIABLE LLVM_LIB_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cflags
                OUTPUT_VARIABLE LLVM_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --system-libs
                OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

message(STATUS "LLVM_INCLUDE_DIR = ${LLVM_INCLUDE_DIR}")
message(STATUS "LLVM_LIB_DIR = ${LLVM_LIB_DIR}")
message(STATUS "LLVM_CFLAGS = ${LLVM_CFLAGS}")
message(STATUS "LLVM_LDFLAGS = ${LLVM_LDFLAGS}")

set(COOGLE_SOURCES
    src/main.cpp
    src/parser.cpp
    src/includes.cpp
)

add_executable(coogle ${COOGLE_SOURCES})
target_include_directories(coogle PRIVATE ${LLVM_INCLUDE_DIR} include)
target_compile_options(coogle PRIVATE ${LLVM_CFLAGS})
target_link_directories(coogle PRIVATE ${LLVM_LIB_DIR})
target_link_libraries(coogle PRIVATE ${LLVM_LDFLAGS} clang fmt::fmt)

# Tests
if(GTest_FOUND)
  enable_testing()

  # Create a library from parser sources (exclude main.cpp)
  add_library(coogle_lib src/parser.cpp src/includes.cpp)
  target_include_directories(coogle_lib PUBLIC include ${LLVM_INCLUDE_DIR})
  target_compile_options(coogle_lib PRIVATE ${LLVM_CFLAGS})
  target_link_directories(coogle_lib PRIVATE ${LLVM_LIB_DIR})
  target_link_libraries(coogle_lib ${LLVM_LDFLAGS} clang fmt::fmt)

  # Test sources
  set(TEST_SOURCES
    test/unit/test_main.cpp
    test/unit/normalize_test.cpp
    test/unit/signature_test.cpp
    test/unit/matching_test.cpp
    test/unit/containers_test.cpp
    test/unit/type_alias_test.cpp
  )

  # Test executable with all test files
  add_executable(coogle_test ${TEST_SOURCES})
  target_link_libraries(coogle_test coogle_lib GTest::gtest)
  target_include_directories(coogle_test PRIVATE include)

  # Register tests with CTest
  add_test(NAME NormalizeTest COMMAND coogle_test --gtest_filter=NormalizeTypeTest.*)
  add_test(NAME SignatureTest COMMAND coogle_test --gtest_filter=ParseSignatureTest.*:ToStringTest.*)
  add_test(NAME MatchingTest COMMAND coogle_test --gtest_filter=SignatureMatchTest.*:WildcardIntegrationTest.*)
  add_test(NAME ContainersTest COMMAND coogle_test --gtest_filter=ContainersTest.*)
  add_test(NAME TypeAliasTest COMMAND coogle_test --gtest_filter=TypeAliasTest.*)
  add_test(NAME AllTests COMMAND coogle_test)

endif()
