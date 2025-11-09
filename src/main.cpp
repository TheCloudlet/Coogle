#include "clang_raii.h"
#include "includes.h"
#include "parser.h"

#include <cassert>
#include <clang-c/Index.h>
#include <cstddef>
#include <format>
#include <iostream>
#include <string>
#include <vector>

// TODO: line number of funciton decl
// TODO: Rename function using LLVM style name

constexpr int EXPECTED_ARG_COUNT = 3;

CXChildVisitResult visitor(CXCursor cursor, [[maybe_unused]] CXCursor parent,
                           CXClientData client_data) {
  auto *targetSig = static_cast<Signature *>(client_data);
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

    if (isSignatureMatch(*targetSig, actual)) {
      // Extract other information
      CXString funcName = clang_getCursorSpelling(cursor);
      const char *funcNameStr = clang_getCString(funcName);

      CXSourceLocation location = clang_getCursorLocation(cursor);
      CXFile file;
      unsigned line = 0, column = 0;
      clang_getSpellingLocation(location, &file, &line, &column, nullptr);

      CXString fileName = clang_getFileName(file);
      const char *fileNameStr = clang_getCString(fileName);

      std::cout << std::format("{:<25}  {:<5}  {:<16}  {}", fileNameStr, line,
                               funcNameStr, toString(actual));

      if (isSignatureMatch(*targetSig, actual)) {
        std::cout << " <--HIT!";
      }

      std::cout << "\n";

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
        "  {} <filename> \"<function_signature_prefix>\"\n\n", argv[0]);
    std::cerr << "Example:\n";
    std::cerr << std::format("  {} example.c \"int(int, char *)\"\n\n",
                             argv[0]);
    return 1;
  }

  const std::string filename = argv[1];
  Signature sig;
  if (!parseFunctionSignature(argv[2], sig)) {
    return 1;
  }

  // Create Clang index with RAII wrapper for automatic cleanup
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

  // Parse translation unit with RAII wrapper for automatic cleanup
  CXTranslationUnitRAII tu(clang_parseTranslationUnit(
      index, filename.c_str(), clangArgs.data(), clangArgs.size(), nullptr, 0,
      CXTranslationUnit_None));
  if (!tu.isValid()) {
    std::cerr << std::format("Error parsing translation unit for file: {}\n",
                             filename);
    return 1; // RAII will automatically clean up index
  }

  std::cout << std::format("\n{:<25}  {:<5}  {:<16}  {}\n", "File", "Line",
                           "Function", "Signature");
  std::cout << std::format("{}\n", std::string(70, '-'));

  CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
  clang_visitChildren(rootCursor, visitor, &sig);

  // RAII wrappers will automatically clean up resources on scope exit
  return 0;
}
