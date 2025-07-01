cmake_minimum_required(VERSION 3.14)
project(Coogle LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -g")

find_package(GTest CONFIG REQUIRED)

find_program(LLVM_CONFIG_EXECUTABLE NAMES llvm-config REQUIRED)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --cflags
                OUTPUT_VARIABLE LLVM_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${LLVM_CONFIG_EXECUTABLE} --libs --system-libs
                OUTPUT_VARIABLE LLVM_LDFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)

include_directories(/opt/homebrew/opt/llvm/include src)
link_directories(/opt/homebrew/opt/llvm/lib)

message(STATUS "LLVM_CFLAGS = ${LLVM_CFLAGS}")
message(STATUS "LLVM_LDFLAGS = ${LLVM_LDFLAGS}")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

add_executable(coogle ${SOURCES})
target_compile_options(coogle PRIVATE ${LLVM_CFLAGS})
target_link_libraries(coogle PRIVATE ${LLVM_LDFLAGS} clang)
