// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Main entry point for Coogle, a C++ function signature search tool.
// Parses source files using libclang and matches function signatures
// with wildcard support.

#include "coogle/arena.h"
#include "coogle/clang_raii.h"
#include "coogle/colors.h"
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
namespace colors = coogle::colors;

constexpr int ExpectedArgCount = 3;

// Supported C/C++ file extensions
constexpr std::array<std::string_view, 8> CppExtensions = {
    ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".hh", ".hxx"};

// Match result with zero-allocation string_view references.
struct Match {
  std::string_view FunctionName; // 16 bytes
  std::string_view SignatureStr; // 16 bytes
  unsigned int Line;             // 4 bytes
} __attribute__((packed));

// Parse results for a single file (flat structure).
struct ParseResults {
  std::string_view FileName;
  std::vector<Match> Matches;
};

// Visitor context with arena storage.
struct VisitorContext {
  coogle::Signature *TargetSig;
  std::string CurrentFile;
  std::vector<ParseResults> *Results;
  coogle::SignatureStorage *Storage; // Arena for match strings
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

  CXCursorKind Kind = clang_getCursorKind(Cursor);
  if (Kind == CXCursor_FunctionDecl || Kind == CXCursor_CXXMethod) {
    // Build actual signature from libclang
    coogle::SignatureStorage ActualStorage;

    // Get return type (canonicalized for semantic type matching)
    CXType RetType = clang_getCursorResultType(Cursor);
    assert(RetType.kind != CXType_Invalid &&
           "Invalid return type obtained from libclang");
    CXType CanonicalRetType = clang_getCanonicalType(RetType);
    CXString RetSpelling = clang_getTypeSpelling(CanonicalRetType);
    std::string_view RetTypeSV = clang_getCString(RetSpelling);
    std::string_view RetTypeInterned = ActualStorage.internString(RetTypeSV);
    std::string_view RetTypeNorm =
        coogle::normalizeType(ActualStorage.arena(), RetTypeInterned);
    clang_disposeString(RetSpelling);

    // Get arguments
    int NumArgs = clang_Cursor_getNumArguments(Cursor);
    ActualStorage.reserveArgs(NumArgs);

    for (int ArgIdx = 0; ArgIdx < NumArgs; ++ArgIdx) {
      CXCursor ArgCursor = clang_Cursor_getArgument(Cursor, ArgIdx);
      assert(!clang_equalCursors(ArgCursor, clang_getNullCursor()) &&
             "Invalid argument cursor obtained from libclang");
      CXType ArgType = clang_getCursorType(ArgCursor);
      assert(ArgType.kind != CXType_Invalid &&
             "Invalid argument type obtained from libclang");
      CXType CanonicalArgType = clang_getCanonicalType(ArgType);
      CXString TypeSpelling = clang_getTypeSpelling(CanonicalArgType);

      std::string_view ArgTypeSV = clang_getCString(TypeSpelling);
      std::string_view ArgTypeInterned = ActualStorage.internString(ArgTypeSV);
      std::string_view ArgTypeNorm =
          coogle::normalizeType(ActualStorage.arena(), ArgTypeInterned);
      ActualStorage.addArg(ArgTypeInterned, ArgTypeNorm);

      clang_disposeString(TypeSpelling);
    }

    // Build signature struct
    coogle::Signature Actual;
    Actual.RetType = RetTypeInterned;
    Actual.RetTypeNorm = RetTypeNorm;
    Actual.ArgTypes = ActualStorage.getArgs();
    Actual.ArgTypesNorm = ActualStorage.getArgsNorm();

    // DEBUG: Print extracted signature (disabled by default)
    // std::cerr << fmt::format("[DEBUG] Function signature: {}\n", coogle::toString(Actual));

    // Check if signature matches
    if (coogle::isSignatureMatch(*Ctx->TargetSig, Actual)) {
      // Get function location
      CXSourceLocation Location = clang_getCursorLocation(Cursor);
      CXFile File;
      unsigned Line = 0;
      clang_getSpellingLocation(Location, &File, &Line, nullptr, nullptr);

      CXString FileName = clang_getFileName(File);
      const char *FileNameStr = clang_getCString(FileName);

      // Only show results from the file we're explicitly parsing
      if (FileNameStr && Ctx->CurrentFile == FileNameStr) {
        // Get function name
        CXString FuncName = clang_getCursorSpelling(Cursor);
        const char *FuncNameStr = clang_getCString(FuncName);

        // Intern strings into the shared storage (immediate copy to arena)
        std::string_view FuncNameView = Ctx->Storage->internString(FuncNameStr);
        std::string SignatureStr = coogle::toString(Actual);
        std::string_view SignatureView =
            Ctx->Storage->internString(SignatureStr);

        // Find or create ParseResults for this file
        auto It = std::find_if(Ctx->Results->begin(), Ctx->Results->end(),
                               [&](const ParseResults &PR) {
                                 return PR.FileName == Ctx->CurrentFile;
                               });

        if (It == Ctx->Results->end()) {
          Ctx->Results->push_back(
              {Ctx->Storage->internString(Ctx->CurrentFile), {}});
          It = Ctx->Results->end() - 1;
        }

        It->Matches.push_back({FuncNameView, SignatureView, Line});

        clang_disposeString(FuncName);
      }

      clang_disposeString(FileName);
    }
  }

  return CXChildVisit_Recurse;
}

int main(int Argc, char *Argv[]) {
  assert(Argv != nullptr && "Argv should not be null");
  assert(Argv[0] != nullptr && "Program name (Argv[0]) should not be null");

  if (Argc != ExpectedArgCount) {
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

  // Parse target signature once (with its own storage)
  coogle::SignatureStorage TargetStorage;
  auto MaybeSig = coogle::parseFunctionSignature(TargetStorage, Argv[2]);
  if (!MaybeSig) {
    return 1;
  }
  coogle::Signature &TargetSig = *MaybeSig;

  // Initialize libclang once
  CXIndexRAII Index;
  if (!Index.isValid()) {
    std::cerr << "Error creating Clang index\n";
    return 1;
  }

  // Don't include system paths - this speeds up parsing dramatically
  // by preventing libclang from parsing all system headers
  std::vector<std::string> ArgsVec;
  ArgsVec.push_back("-x");
  ArgsVec.push_back("c++");
  ArgsVec.push_back("-nostdinc");   // Don't search standard system directories
  ArgsVec.push_back("-nostdinc++"); // Don't search standard C++ directories

  std::vector<const char *> ClangArgs;
  for (const auto &S : ArgsVec) {
    ClangArgs.push_back(S.c_str());
  }

  // --- Data Collection ---
  // Single storage for all match strings
  coogle::SignatureStorage MatchStorage;
  std::vector<ParseResults> Results;
  std::vector<std::string> ParseFailures;

  // Parse each file
  for (const auto &Filename : Files) {
    // Performance optimization: Skip function bodies and detailed preprocessing
    // We only need function signatures, not implementations
    unsigned Options =
        CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_Incomplete;

    CXTranslationUnitRAII TU(
        clang_parseTranslationUnit(Index, Filename.c_str(), ClangArgs.data(),
                                   ClangArgs.size(), nullptr, 0, Options));

    if (!TU.isValid()) {
      ParseFailures.push_back(Filename);
      continue;
    }

    VisitorContext Ctx{&TargetSig, Filename, &Results, &MatchStorage};
    CXCursor RootCursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(RootCursor, visitor, &Ctx);
  }

  // --- Output ---
  fmt::print("\n{}▶ Searching for: {}{}\n\n", colors::Bold,
             coogle::toString(TargetSig), colors::Reset);

  int TotalMatches = 0;
  for (const auto &Result : Results) {
    fmt::print("{}{}✔ {}{}\n", colors::Bold, colors::Blue, Result.FileName,
               colors::Reset);
    for (const auto &Match : Result.Matches) {
      fmt::print("  {}└─ {}{}: {}{}{}{}\n", colors::Grey, colors::Yellow,
                 Match.Line, colors::Reset, colors::Green, Match.FunctionName,
                 colors::Reset, Match.SignatureStr);
      TotalMatches++;
    }
  }

  for (const auto &File : ParseFailures) {
    fmt::print("{}{}✖ Warning: {}{}Failed to parse {}\n", colors::Bold,
               colors::Yellow, colors::Reset, File);
  }

  fmt::print("\nMatches found: {}\n", TotalMatches);

  return 0;
}
