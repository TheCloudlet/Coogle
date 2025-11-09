#include "clang_raii.h"
#include "includes.h"
#include "parser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <clang-c/Index.h>
#include <cstddef>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

// TODO: line number of funciton decl
// TODO: Rename function using LLVM style name

constexpr int EXPECTED_ARG_COUNT = 3;

// Supported C/C++ file extensions
constexpr std::array<std::string_view, 8> CPP_EXTENSIONS = {
    ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"};

struct VisitorContext {
  Signature *targetSig;
  std::string currentFile;
  int *matchCount;
};

// Find all C/C++ source files in the given path
std::vector<std::string> findSourceFiles(const fs::path &path) {
  std::vector<std::string> files;

  if (fs::is_regular_file(path)) {
    // Single file mode
    files.push_back(path.string());
  } else if (fs::is_directory(path)) {
    // Directory mode - recursive search
    for (const auto &entry : fs::recursive_directory_iterator(
             path, fs::directory_options::skip_permission_denied)) {
      if (entry.is_regular_file()) {
        auto ext = entry.path().extension().string();
        if (std::find(CPP_EXTENSIONS.begin(), CPP_EXTENSIONS.end(), ext) !=
            CPP_EXTENSIONS.end()) {
          files.push_back(entry.path().string());
        }
      }
    }
  }

  return files;
}

CXChildVisitResult visitor(CXCursor cursor, [[maybe_unused]] CXCursor parent,
                           CXClientData client_data) {
  auto *ctx = static_cast<VisitorContext *>(client_data);
  Signature actual;

  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
    CXType retType = clang_getCursorResultType(cursor);
    CXString retSpelling = clang_getTypeSpelling(retType);

    actual.retType = clang_getCString(retSpelling);
    clang_disposeString(retSpelling);

    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; ++i) {
      CXCursor argCursor = clang_Cursor_getArgument(cursor, i);
      CXString argName = clang_getCursorSpelling(argCursor); // Not necessary
      CXType argType = clang_getCursorType(argCursor);
      CXString typeSpelling = clang_getTypeSpelling(argType);

      actual.argType.push_back(clang_getCString(typeSpelling));

      clang_disposeString(argName);
      clang_disposeString(typeSpelling);
    }

    if (isSignatureMatch(*ctx->targetSig, actual)) {
      // Extract other information
      CXString funcName = clang_getCursorSpelling(cursor);
      const char *funcNameStr = clang_getCString(funcName);

      CXSourceLocation location = clang_getCursorLocation(cursor);
      CXFile file;
      unsigned line = 0, column = 0;
      clang_getSpellingLocation(location, &file, &line, &column, nullptr);

      CXString fileName = clang_getFileName(file);
      const char *fileNameStr = clang_getCString(fileName);

      // Only show results from the file we're explicitly parsing
      // This filters out system headers automatically
      if (fileNameStr && ctx->currentFile == fileNameStr) {
        std::cout << std::format("{:<40}  {:<5}  {:<20}  {}\n", fileNameStr,
                                 line, funcNameStr, toString(actual));
        (*ctx->matchCount)++;
      }

      clang_disposeString(funcName);
      clang_disposeString(fileName);
    }
  }

  return CXChildVisit_Recurse;
}

int main(int argc, char *argv[]) {
  if (argc != EXPECTED_ARG_COUNT) { // Use constant for argument count
    std::cerr << std::format("✖ Error: Incorrect number of arguments.\n\n");
    std::cerr << "Usage:\n";
    std::cerr << std::format(
        "  {} <file_or_directory> \"<function_signature>\"\n\n", argv[0]);
    std::cerr << "Examples:\n";
    std::cerr << std::format("  {} example.c \"int(int, char *)\"\n", argv[0]);
    std::cerr << std::format("  {} src/ \"void(char *)\"\n", argv[0]);
    std::cerr << std::format("  {} . \"int(*)(void)\"\n\n", argv[0]);
    return 1;
  }

  const std::string inputPath = argv[1];
  fs::path path(inputPath);

  // Check if path exists
  if (!fs::exists(path)) {
    std::cerr << std::format("Error: Path '{}' does not exist\n", inputPath);
    return 1;
  }

  // Discover files to parse
  std::vector<std::string> files = findSourceFiles(path);

  if (files.empty()) {
    std::cerr << std::format("No C/C++ files found in: {}\n", inputPath);
    return 1;
  }

  // Parse signature once
  Signature sig;
  if (!parseFunctionSignature(argv[2], sig)) {
    return 1;
  }

  // Initialize libclang once
  CXIndexRAII index;
  if (!index.isValid()) {
    std::cerr << "Error creating Clang index\n";
    return 1;
  }

  std::vector<std::string> argsVec = detectSystemIncludePaths();
  std::vector<const char *> clangArgs;
  for (const auto &s : argsVec) {
    clangArgs.push_back(s.c_str());
  }

  // Print header
  std::cout << std::format("\nSearching {} file(s) for signature: {}\n\n",
                           files.size(), toString(sig));
  std::cout << std::format("{:<40}  {:<5}  {:<20}  {}\n", "File", "Line",
                           "Function", "Signature");
  std::cout << std::format("{}\n", std::string(100, '-'));

  int totalMatches = 0;

  // Parse each file
  for (const auto &filename : files) {
    CXTranslationUnitRAII tu(clang_parseTranslationUnit(
        index, filename.c_str(), clangArgs.data(), clangArgs.size(), nullptr, 0,
        CXTranslationUnit_None));

    if (!tu.isValid()) {
      std::cerr << std::format("Warning: Failed to parse {}\n", filename);
      continue;
    }

    VisitorContext ctx{&sig, filename, &totalMatches};
    CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
    clang_visitChildren(rootCursor, visitor, &ctx);
  }

  std::cout << std::format("\nTotal matches: {}\n", totalMatches);

  // RAII wrappers will automatically clean up resources on scope exit
  return 0;
}
