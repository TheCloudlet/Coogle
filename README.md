# Coogle

**Coogle** is a lightweight C++ command-line tool for searching C/C++ functions based on their type signatures — inspired by [Hoogle](https://hoogle.haskell.org) from the Haskell ecosystem.

## Overview

In large C/C++ codebases — especially legacy systems or unfamiliar third-party libraries — it's often difficult to locate the right function just by grepping filenames or browsing header files. **Coogle** helps by allowing you to search functions using partial or full type signatures.

### Features

- Quickly locate functions that match a specific input/output structure
- Navigate unfamiliar or legacy codebases with ease
- Explore large libraries without reading every header manually
- Discover reusable APIs and learn from existing patterns
- Integrate function-based search into your editor or workflow

## Requirements

- C++20 compiler (GCC 10+, Clang 10+, or MSVC 19.26+)
- CMake 3.14 or later
- [libclang](https://clang.llvm.org/docs/Tooling.html#libclang) - LLVM/Clang tooling library
- [GoogleTest](https://github.com/google/googletest) - optional, for unit testing

## Installation

### 1. Clone the repository

```bash
git clone https://github.com/TheCloudlet/Coogle
cd coogle
```

### 2. Install LLVM (macOS example)

```bash
brew install llvm
```

### 3. Configure environment variables (macOS with Homebrew LLVM)

Add these to your shell config (`~/.zshrc`, `~/.bash_profile`, etc.):

```bash
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
export LDFLAGS="-L/opt/homebrew/opt/llvm/lib"
export CPPFLAGS="-I/opt/homebrew/opt/llvm/include"
export CPATH="/opt/homebrew/opt/llvm/include/c++/v1"
```

Then apply the settings:

```bash
source ~/.zshrc  # or source ~/.bash_profile
```

**Note:** If you see errors like `fatal error: 'string' file not found`, make sure your environment is properly configured.

### 4. Build the project

```bash
mkdir -p build
cd build
cmake ..
make
```

This will generate the `coogle` executable inside the `build/` directory.

## Usage

```bash
./build/coogle <source_file> "<function_signature>"
```

### Signature Format

Signatures follow the format:

```text
return_type(arg1_type, arg2_type, ...)
```

For example, `int(char *, int)` matches any function returning `int` and taking two arguments: `char *` and `int`.

### Example

```bash
./build/coogle test/inputs/example.c "int(int, int)"
```

## Architecture

Coogle uses libclang to parse C/C++ source files and extract function signatures:

1. **AST Parsing**: Uses Clang's C API to build an abstract syntax tree
2. **System Include Detection**: Automatically detects system include paths via `clang -E -v`
3. **Visitor Pattern**: Implements a cursor visitor to walk the AST and extract function declarations
4. **Type Normalization**: Normalizes types for flexible matching (ignoring whitespace and `const` qualifiers)
5. **RAII Resource Management**: Uses custom RAII wrappers for safe libclang resource handling

### Recent Improvements

**Performance & Correctness (2025)**

- Fixed critical bug in signature matching that caused all functions to be displayed
- Optimized type normalization by replacing regex with single-pass character parsing
- Implemented RAII wrappers (`CXIndexRAII`, `CXTranslationUnitRAII`, `CXStringRAII`) for memory safety
- Upgraded to C++20 and replaced C-style `printf` with type-safe `std::format`

## Implementation Status

**Completed:**

- [x] Uses Clang's C API (`libclang`) to parse translation units
- [x] Detects system include paths automatically via `clang -E -v`
- [x] Implements visitor pattern to walk the AST and extract function signatures
- [x] Enables strict argument matching with type normalization
- [x] RAII-based resource management for libclang objects

**Planned:**

- [ ] Unit tests using GoogleTest
- [ ] Support for files in different directories `coogle src/ "int(char *, int)"`
  - [ ] Unified support `file mode` and `directory mode`
- [ ] Wildcard-style queries (e.g., `int(char *, *)`)
- [ ] Recursive search across multiple files

## License

MIT License. See [LICENSE](LICENSE) for details.
