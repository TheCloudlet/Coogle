cmake_minimum_required(VERSION 3.14)
project(Coogle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

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

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(coogle ${SOURCES})
target_include_directories(coogle PRIVATE ${LLVM_INCLUDE_DIR} src)
target_compile_options(coogle PRIVATE ${LLVM_CFLAGS})
target_link_directories(coogle PRIVATE ${LLVM_LIB_DIR})
target_link_libraries(coogle PRIVATE ${LLVM_LDFLAGS} clang)
