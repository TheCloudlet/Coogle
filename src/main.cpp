#include "coogle/clang_raii.h"
#include "coogle/includes.h"
#include "coogle/parser.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <clang-c/Index.h>
#include <cstddef>
#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;

constexpr int ExpectedArgCount = 3;

// Supported C/C++ file extensions
constexpr std::array<std::string_view, 8> CppExtensions = {
    ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"};

struct VisitorContext {
  Signature *TargetSig;
  std::string CurrentFile;
  int *MatchCount;
};

// Find all C/C++ source files in the given path
std::vector<std::string> findSourceFiles(const fs::path &Path) {
  std::vector<std::string> Files;

  if (fs::is_regular_file(Path)) {
    // Single file mode
    Files.push_back(Path.string());
  } else if (fs::is_directory(Path)) {
    // Directory mode - recursive search
    for (const auto &Entry : fs::recursive_directory_iterator(
             Path, fs::directory_options::skip_permission_denied)) {
      if (Entry.is_regular_file()) {
        auto Ext = Entry.path().extension().string();
        if (std::find(CppExtensions.begin(), CppExtensions.end(), Ext) !=
            CppExtensions.end()) {
          Files.push_back(Entry.path().string());
        }
      }
    }
  }

  return Files;
}

CXChildVisitResult visitor(CXCursor Cursor, [[maybe_unused]] CXCursor Parent,
                           CXClientData ClientData) {
  auto *Ctx = static_cast<VisitorContext *>(ClientData);
  Signature Actual;

  CXCursorKind Kind = clang_getCursorKind(Cursor);
  if (Kind == CXCursor_FunctionDecl || Kind == CXCursor_CXXMethod) {
    CXType RetType = clang_getCursorResultType(Cursor);
    assert(RetType.kind != CXType_Invalid &&
           "Invalid return type obtained from libclang");
    CXString RetSpelling = clang_getTypeSpelling(RetType);

    Actual.RetType = clang_getCString(RetSpelling);
    clang_disposeString(RetSpelling);

    int NumArgs = clang_Cursor_getNumArguments(Cursor);
    for (int ArgIdx = 0; ArgIdx < NumArgs; ++ArgIdx) {
      CXCursor ArgCursor = clang_Cursor_getArgument(Cursor, ArgIdx);
      assert(!clang_equalCursors(ArgCursor, clang_getNullCursor()) &&
             "Invalid argument cursor obtained from libclang");
      CXString ArgName = clang_getCursorSpelling(ArgCursor); // Not necessary
      CXType ArgType = clang_getCursorType(ArgCursor);
      assert(ArgType.kind != CXType_Invalid &&
             "Invalid argument type obtained from libclang");
      CXString TypeSpelling = clang_getTypeSpelling(ArgType);

      Actual.ArgType.push_back(clang_getCString(TypeSpelling));

      clang_disposeString(ArgName);
      clang_disposeString(TypeSpelling);
    }

    if (isSignatureMatch(*Ctx->TargetSig, Actual)) {
      // Extract other information
      CXString FuncName = clang_getCursorSpelling(Cursor);
      const char *FuncNameStr = clang_getCString(FuncName);

      CXSourceLocation Location = clang_getCursorLocation(Cursor);
      CXFile File;
      unsigned Line = 0, Column = 0;
      clang_getSpellingLocation(Location, &File, &Line, &Column, nullptr);

      CXString FileName = clang_getFileName(File);
      const char *FileNameStr = clang_getCString(FileName);

      // Only show results from the file we're explicitly parsing
      // This filters out system headers automatically
      if (FileNameStr && Ctx->CurrentFile == FileNameStr) {
        std::cout << fmt::format("{:<40}  {:<5}  {:<20}  {}\n", FileNameStr,
                                 Line, FuncNameStr, toString(Actual));
        (*Ctx->MatchCount)++;
      }

      clang_disposeString(FuncName);
      clang_disposeString(FileName);
    }
  }

  return CXChildVisit_Recurse;
}

int main(int Argc, char *Argv[]) {
  assert(Argv != nullptr && "Argv should not be null");
  assert(Argv[0] != nullptr && "Program name (Argv[0]) should not be null");

  if (Argc != ExpectedArgCount) { // Use constant for argument count
    std::cerr << fmt::format("✖ Error: Incorrect number of arguments.\n\n");
    std::cerr << "Usage:\n";
    std::cerr << fmt::format(
        "  {} <file_or_directory> \"<function_signature>\"\n\n", Argv[0]);
    std::cerr << "Examples:\n";
    std::cerr << fmt::format("  {} example.c \"int(int, char *)\"\n", Argv[0]);
    std::cerr << fmt::format("  {} src/ \"void(char *)\"\n", Argv[0]);
    std::cerr << fmt::format("  {} . \"int(*)(void)\"\n\n", Argv[0]);
    return 1;
  }

  // Assertions for programming errors, if Argv[1] or Argv[2] are null
  // despite Argc being correct.
  assert(Argv[1] != nullptr &&
         "Input path argument (Argv[1]) should not be null");
  assert(Argv[2] != nullptr &&
         "Signature argument (Argv[2]) should not be null");

  const std::string InputPath = Argv[1];
  fs::path Path(InputPath);

  // Check if path exists
  if (!fs::exists(Path)) {
    std::cerr << fmt::format("Error: Path '{}' does not exist\n", InputPath);
    return 1;
  }

  // Discover files to parse
  std::vector<std::string> Files = findSourceFiles(Path);

  if (Files.empty()) {
    std::cerr << fmt::format("No C/C++ files found in: {}\n", InputPath);
    return 1;
  }

  // Parse signature once
  Signature Sig;
  if (!parseFunctionSignature(Argv[2], Sig)) {
    return 1;
  }

  // Initialize libclang once
  CXIndexRAII Index;
  if (!Index.isValid()) {
    std::cerr << "Error creating Clang index\n";
    return 1;
  }

  std::vector<std::string> ArgsVec = detectSystemIncludePaths();
  std::vector<const char *> ClangArgs;
  for (const auto &S : ArgsVec) {
    ClangArgs.push_back(S.c_str());
  }

  // Print header
  std::cout << fmt::format("\nSearching {} file(s) for signature: {}\n\n",
                           Files.size(), toString(Sig));
  std::cout << fmt::format("{:<40}  {:<5}  {:<20}  {}\n", "File", "Line",
                           "Function", "Signature");
  std::cout << fmt::format("{}\n", std::string(100, '-'));

  int TotalMatches = 0;

  // Parse each file
  for (const auto &Filename : Files) {
    CXTranslationUnitRAII TU(clang_parseTranslationUnit(
        Index, Filename.c_str(), ClangArgs.data(), ClangArgs.size(), nullptr, 0,
        CXTranslationUnit_None));

    if (!TU.isValid()) {
      std::cerr << fmt::format("Warning: Failed to parse {}\n", Filename);
      continue;
    }

    VisitorContext Ctx{&Sig, Filename, &TotalMatches};
    CXCursor RootCursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(RootCursor, visitor, &Ctx);
  }

  std::cout << fmt::format("\nTotal matches: {}\n", TotalMatches);

  // RAII wrappers will automatically clean up resources on scope exit
  return 0;
}
