# Coogle

**Coogle** is a lightweight C++ command-line tool for searching C/C++ functions based on their type signatures — inspired by [Hoogle](https://hoogle.haskell.org) from the Haskell ecosystem.

## 🌟 Why Use It?

In large C/C++ codebases — especially legacy systems or unfamiliar third-party libraries — it’s often difficult to locate the right function just by grepping filenames or browsing header files. **Coogle** helps by allowing you to search functions using partial or full type signatures.

It enables you to:

- 🔍 Quickly locate functions that match a specific input/output structure  
- 🧭 Navigate unfamiliar or legacy codebases with ease  
- 📚 Explore large libraries without reading every header manually  
- 🧠 Discover reusable APIs and learn from existing patterns  
- ⚙️ Integrate function-based search into your editor or workflow  

## 📦 Dependencies

- [`libclang`](https://clang.llvm.org/docs/Tooling.html#libclang)
- [GoogleTest](https://github.com/google/googletest) (optional, for unit testing)

## 🚀 Installation

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

> ⚠️ If you see errors like `fatal error: 'string' file not found`, make sure your environment is properly configured.

### 4. Build the project

```bash
mkdir -p build
cd build
cmake ..
make
```

This will generate the `coogle` executable inside the `build/` directory.

## 🛠 Usage

```bash
./build/coogle path/to/source.cpp "type signatures"
```
Sigature is like

```text
int(char *, int)
```
...to match any function returning `int` and taking two arguments. The first is `char *`, and the second is `int`

Example:

```bash
./build/coogle ./test/input/example "int(int, int)"
```

## 🧩 Implementation Notes

- [x] Uses Clang’s C API (`libclang`) to parse translation units
- [x] Detects system include paths automatically via `clang -E -v`
- [x] Implements a simple visitor pattern to walk the AST and extract function signatures
- [x] Enables strict argument matching
- [ ] Write tests to ensure code quality
- [ ] Supports wildcard-style queries (e.g. `int(char *, *)`)
- [ ] Supports recursive search across multiple files

## 📜 License

MIT License. See [`LICENSE`](LICENSE) for details.
