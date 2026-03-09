cmake_minimum_required(VERSION 3.14)
project(Coogle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

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
if(NOT GTest_FOUND)
  message(STATUS "GTest not found locally, fetching from GitHub...")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
  )
  # For Windows: prevent overriding the parent project's compiler/linker settings
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
  
  # Map fetched targets to the expected GTest:: names if they don't exist
  if(NOT TARGET GTest::gtest)
    add_library(GTest::gtest ALIAS gtest)
  endif()
  if(NOT TARGET GTest::gtest_main)
    add_library(GTest::gtest_main ALIAS gtest_main)
  endif()
  set(GTest_FOUND TRUE)
endif()

find_package(Threads REQUIRED)

# Robustly find llvm-config that has matching clang headers
set(LLVM_VERSIONS 19 18 17 16 15 14 13)
foreach(VER ${LLVM_VERSIONS})
  find_program(LLVM_CONFIG_TEMP NAMES llvm-config-${VER} llvm-config)
  if(LLVM_CONFIG_TEMP)
    execute_process(COMMAND ${LLVM_CONFIG_TEMP} --includedir
                    OUTPUT_VARIABLE TEMP_INC_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(EXISTS "${TEMP_INC_DIR}/clang-c/Index.h")
      set(LLVM_CONFIG_EXECUTABLE ${LLVM_CONFIG_TEMP} CACHE FILEPATH "Path to a program.")
      message(STATUS "Found compatible LLVM/Clang version: ${VER} at ${LLVM_CONFIG_EXECUTABLE}")
      break()
    endif()
  endif()
  unset(LLVM_CONFIG_TEMP CACHE)
endforeach()

if(NOT LLVM_CONFIG_EXECUTABLE)
  # Fallback to standard search if loop failed
  find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config REQUIRED)
endif()

execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --includedir
                OUTPUT_VARIABLE LLVM_INCLUDE_DIR_STR OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(LLVM_INCLUDE_DIR NATIVE_COMMAND "${LLVM_INCLUDE_DIR_STR}")

execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libdir
                OUTPUT_VARIABLE LLVM_LIB_DIR_STR OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(LLVM_LIB_DIR NATIVE_COMMAND "${LLVM_LIB_DIR_STR}")

execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cflags
                OUTPUT_VARIABLE LLVM_CFLAGS_STR OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --system-libs
                OUTPUT_VARIABLE LLVM_LDFLAGS_STR OUTPUT_STRIP_TRAILING_WHITESPACE)

# Filter out -stdlib=libc++ on Linux to prevent issues with GCC
if(NOT APPLE)
  string(REPLACE "-stdlib=libc++" "" LLVM_CFLAGS_STR "${LLVM_CFLAGS_STR}")
  string(REPLACE "-stdlib=libc++" "" LLVM_LDFLAGS_STR "${LLVM_LDFLAGS_STR}")
endif()

separate_arguments(LLVM_CFLAGS NATIVE_COMMAND "${LLVM_CFLAGS_STR}")
separate_arguments(LLVM_LDFLAGS NATIVE_COMMAND "${LLVM_LDFLAGS_STR}")

message(STATUS "LLVM_INCLUDE_DIR = ${LLVM_INCLUDE_DIR_STR}")
message(STATUS "LLVM_LIB_DIR = ${LLVM_LIB_DIR_STR}")

set(COOGLE_SOURCES
    src/main.cpp
    src/parser.cpp
    src/includes.cpp
)

add_executable(coogle ${COOGLE_SOURCES})
target_include_directories(coogle SYSTEM PRIVATE ${LLVM_INCLUDE_DIR} include)
target_compile_options(coogle PRIVATE ${LLVM_CFLAGS})
target_link_directories(coogle PRIVATE ${LLVM_LIB_DIR})
target_link_libraries(coogle PRIVATE ${LLVM_LDFLAGS} clang fmt::fmt Threads::Threads)

# Tests
if(GTest_FOUND)
  enable_testing()

  # Create a library from parser sources (exclude main.cpp)
  add_library(coogle_lib src/parser.cpp src/includes.cpp)
  target_include_directories(coogle_lib SYSTEM PUBLIC ${LLVM_INCLUDE_DIR})
  target_include_directories(coogle_lib PUBLIC include)
  target_compile_options(coogle_lib PUBLIC ${LLVM_CFLAGS})
  target_link_directories(coogle_lib PUBLIC ${LLVM_LIB_DIR})
  target_link_libraries(coogle_lib PUBLIC ${LLVM_LDFLAGS} clang fmt::fmt Threads::Threads)

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
  target_link_libraries(coogle_test PRIVATE coogle_lib GTest::gtest)
  target_include_directories(coogle_test PRIVATE include)

  # Register tests with CTest
  add_test(NAME NormalizeTest COMMAND coogle_test --gtest_filter=NormalizeTypeTest.*)
  add_test(NAME SignatureTest COMMAND coogle_test --gtest_filter=ParseSignatureTest.*:ToStringTest.*)
  add_test(NAME MatchingTest COMMAND coogle_test --gtest_filter=SignatureMatchTest.*:WildcardIntegrationTest.*)
  add_test(NAME ContainersTest COMMAND coogle_test --gtest_filter=ContainersTest.*)
  add_test(NAME TypeAliasTest COMMAND coogle_test --gtest_filter=TypeAliasTest.*)
  add_test(NAME AllTests COMMAND coogle_test)

endif()
