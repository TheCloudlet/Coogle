#include "includes.h"
#include "parser.h"
#include <cassert>
#include <clang-c/Index.h>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

// TODO: line number of funciton decl
// TODO: Rename function using LLVM style name

constexpr int EXPECTED_ARG_COUNT = 3;

CXChildVisitResult visitor(CXCursor cursor, CXCursor parent,
                           CXClientData client_data) {
  auto* targetSig = static_cast<Signature*>(client_data);
  Signature actual;

  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_FunctionDecl || kind == CXCursor_CXXMethod) {
    CXString funcName = clang_getCursorSpelling(cursor);
    CXType retType = clang_getCursorResultType(cursor);
    CXString retSpelling = clang_getTypeSpelling(retType);

    actual.funcName = clang_getCString(funcName);
    actual.retType = clang_getCString(retSpelling);

    // clang-format off
    std::printf("%s: %s(", clang_getCString(funcName), clang_getCString(retSpelling));
    // clang-format on

    clang_disposeString(funcName);
    clang_disposeString(retSpelling);

    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; ++i) {
      CXCursor argCursor = clang_Cursor_getArgument(cursor, i);
      CXString argName = clang_getCursorSpelling(argCursor);
      CXType argType = clang_getCursorType(argCursor);
      CXString typeSpelling = clang_getTypeSpelling(argType);
      actual.argType.push_back(clang_getCString(typeSpelling));

      std::printf("%s", clang_getCString(typeSpelling));
      if (i != numArgs - 1)
        std::printf(",");

      clang_disposeString(argName);
      clang_disposeString(typeSpelling);
    }
    std::printf(")\n");

    if (isSignatureMatch(*targetSig, actual)) {
      std::printf("✔ MATCH: %s\n", actual.funcName.c_str());
    } else {
      std::printf("✖ MISMATCH: %s\n", actual.funcName.c_str());
    }
  }

  return CXChildVisit_Recurse;
}

int main(int argc, char *argv[]) {
  if (argc != EXPECTED_ARG_COUNT) { // Use constant for argument count
    // clang-format off
    std::fprintf(stderr, "✖ Error: Incorrect number of arguments.\n\n");
    std::fprintf(stderr, "Usage:\n");
    std::fprintf(stderr, "  %s <filename> \"<function_signature_prefix>\"\n\n", argv[0]);
    std::fprintf(stderr, "Example:\n");
    std::fprintf(stderr, "  %s example.c \"int(int, char *)\"\n\n", argv[0]);
    // clang-format on
    return 1;
  }

  const std::string filename = argv[1];
  Signature sig;
  if (!parseFunctionSignature(argv[2], sig)) {
    return 1;
  }

  CXIndex index = clang_createIndex(0, 0);
  if (!index) {
    std::fprintf(stderr, "Error creating Clang index\n");
    return 1;
  }

  std::vector<std::string> argsVec = detectSystemIncludePaths();
  std::vector<const char *> clangArgs;
  for (const auto &s : argsVec) {
    clangArgs.push_back(s.c_str());
  }

  CXTranslationUnit tu = clang_parseTranslationUnit(
      index, filename.c_str(), clangArgs.data(), clangArgs.size(), nullptr, 0,
      CXTranslationUnit_None);
  if (!tu) { // Check for null translation unit
    // clang-fomat off
    std::fprintf(stderr, "Error parsing translation unit for file: %s\n",
                 filename.c_str());
    // clang-fomat on
    clang_disposeIndex(index);
    return 1;
  }

  CXCursor rootCursor = clang_getTranslationUnitCursor(tu);
  clang_visitChildren(rootCursor, visitor, &sig);

  clang_disposeTranslationUnit(tu); // Clean up translation unit
  clang_disposeIndex(index);        // Clean up Clang index

  return 0;
}
