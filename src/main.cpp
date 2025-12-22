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
#include <future>
#include <iostream>
#include <numeric>
#include <string>
#include <string_view>
#include <thread>
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

// Result of a processing task (thread-local storage)
struct TaskResult {
  coogle::SignatureStorage Storage; // Owns the strings in Results
  std::vector<ParseResults> Results;
  std::vector<std::string> Failures;

  // Enable move
  TaskResult() = default;
  TaskResult(TaskResult &&) = default;
  TaskResult &operator=(TaskResult &&) = default;
  TaskResult(const TaskResult &) = delete;
  TaskResult &operator=(const TaskResult &) = delete;
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
      if (clang_equalCursors(ArgCursor, clang_getNullCursor())) {
        continue; // Skip invalid arguments (e.g. variadic args sometimes cause
                  // issues)
      }
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

    // Check if signature matches
    if (coogle::isSignatureMatch(*Ctx->TargetSig, Actual)) {
      // Get function location
      CXSourceLocation Location = clang_getCursorLocation(Cursor);

      // Skip system headers (double protection)
      if (clang_Location_isInSystemHeader(Location)) {
        return CXChildVisit_Continue;
      }

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

        // Optimization: Since we process files sequentially, we only need to
        // check the last entry
        bool IsNewFile = Ctx->Results->empty() ||
                         Ctx->Results->back().FileName != Ctx->CurrentFile;

        if (IsNewFile) {
          Ctx->Results->push_back(
              {Ctx->Storage->internString(Ctx->CurrentFile), {}});
        }

        Ctx->Results->back().Matches.push_back(
            {FuncNameView, SignatureView, Line});

        clang_disposeString(FuncName);
      }

      clang_disposeString(FileName);
    }
  }

  return CXChildVisit_Recurse;
}

TaskResult processFiles(const std::vector<std::string> &Files,
                        const coogle::Signature &TargetSig,
                        const std::vector<const char *> &ClangArgs) {
  TaskResult Result;

  // Each thread needs its own index to avoid contention
  CXIndexRAII Index;
  if (!Index.isValid()) {
    std::cerr << "Error creating Clang index in worker thread\n";
    return Result;
  }

  for (const auto &Filename : Files) {
    // Performance optimization:
    // - SkipFunctionBodies: We only need signatures
    // - Incomplete: Allow parsing errors (missing headers)
    // - SingleFileParse: Don't process included headers (we don't want them
    // anyway)
    // - LimitSkipFunctionBodiesToPreamble: Further optim for skipping bodies
    unsigned Options =
        CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_Incomplete |
        CXTranslationUnit_SingleFileParse | CXTranslationUnit_KeepGoing;

    CXTranslationUnitRAII TU(
        clang_parseTranslationUnit(Index, Filename.c_str(), ClangArgs.data(),
                                   ClangArgs.size(), nullptr, 0, Options));

    if (!TU.isValid()) {
      Result.Failures.push_back(Filename);
      continue;
    }

    VisitorContext Ctx{const_cast<coogle::Signature *>(&TargetSig), Filename,
                       &Result.Results, &Result.Storage};
    CXCursor RootCursor = clang_getTranslationUnitCursor(TU);
    clang_visitChildren(RootCursor, visitor, &Ctx);
  }

  return Result;
}

void printHelp(const char *ProgramName) {
  std::cout << fmt::format("Coogle - C++ Function Signature Search Tool\n\n");
  std::cout << fmt::format("Usage:\n");
  std::cout << fmt::format(
      "  {} <file_or_directory> \"<function_signature>\"\n", ProgramName);
  std::cout << fmt::format("  {} --help\n\n", ProgramName);
  std::cout << fmt::format("Arguments:\n");
  std::cout << fmt::format(
      "  <file_or_directory>     C/C++ source file or directory to search\n");
  std::cout << fmt::format(
      "  <function_signature>    Function signature pattern to match\n\n");
  std::cout << fmt::format("Signature Format:\n");
  std::cout << fmt::format("  return_type(arg1_type, arg2_type, ...)\n\n");
  std::cout << fmt::format("Wildcards:\n");
  std::cout << fmt::format("  Use '*' to match any argument type\n");
  std::cout << fmt::format(
      "  Example: \"void(*, int)\" matches any function returning void\n");
  std::cout << fmt::format(
      "           with any first argument and int second argument\n\n");
  std::cout << fmt::format("Examples:\n");
  std::cout << fmt::format("  {} example.c \"int(int, char *)\"\n",
                           ProgramName);
  std::cout << fmt::format("  {} src/ \"void(char *)\"\n", ProgramName);
  std::cout << fmt::format("  {} . \"int(*)(void)\"\n", ProgramName);
  std::cout << fmt::format(
      "  {} main.cpp \"std::string(const std::string &)\"\n", ProgramName);
  std::cout << fmt::format(
      "  {} . \"void(*, *)\"  # Find all void functions with 2 args\n\n",
      ProgramName);
  std::cout << fmt::format("Features:\n");
  std::cout << fmt::format("  • Parallel file processing\n");
  std::cout << fmt::format("  • Canonical type resolution\n");
  std::cout << fmt::format("  • Template-aware matching\n");
  std::cout << fmt::format("  • System header filtering\n");
  std::cout << fmt::format("  • Wildcard argument matching\n\n");
}

int main(int Argc, char *Argv[]) {
  assert(Argv != nullptr && "Argv should not be null");
  assert(Argv[0] != nullptr && "Program name (Argv[0]) should not be null");

  // Check for --help flag
  if (Argc == 2 &&
      (std::string(Argv[1]) == "--help" || std::string(Argv[1]) == "-h")) {
    printHelp(Argv[0]);
    return 0;
  }

  if (Argc != ExpectedArgCount) {
    std::cerr << fmt::format("✖ Error: Incorrect number of arguments.\n\n");
    std::cerr << "Usage:\n";
    std::cerr << fmt::format(
        "  {} <file_or_directory> \"<function_signature>\"\n", Argv[0]);
    std::cerr << fmt::format("  {} --help\n\n", Argv[0]);
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

  // Prepare clang args
  std::vector<std::string> ArgsVec;
  ArgsVec.push_back("-x");
  ArgsVec.push_back("c++");
  ArgsVec.push_back("-nostdinc");   // Don't search standard system directories
  ArgsVec.push_back("-nostdinc++"); // Don't search standard C++ directories

  std::vector<const char *> ClangArgs;
  for (const auto &S : ArgsVec) {
    ClangArgs.push_back(S.c_str());
  }

  // Determine thread count and chunk size
  const size_t NumThreads = std::max(1u, std::thread::hardware_concurrency());
  const size_t NumFiles = Files.size();
  const size_t ChunkSize = (NumFiles + NumThreads - 1) / NumThreads;

  std::vector<std::future<TaskResult>> Futures;

  // Launch tasks
  for (size_t i = 0; i < NumThreads; ++i) {
    size_t Start = i * ChunkSize;
    if (Start >= NumFiles)
      break;
    size_t End = std::min(Start + ChunkSize, NumFiles);

    // Copy subset of files for this chunk
    std::vector<std::string> ChunkFiles(Files.begin() + Start,
                                        Files.begin() + End);

    Futures.push_back(std::async(std::launch::async, processFiles,
                                 std::move(ChunkFiles), std::cref(TargetSig),
                                 std::cref(ClangArgs)));
  }

  // Collect results
  std::vector<TaskResult> AllResults;
  for (auto &Fut : Futures) {
    AllResults.push_back(Fut.get());
  }

  // --- Output ---
  fmt::print("\n{}▶ Searching for: {}{}\n\n", colors::Bold,
             coogle::toString(TargetSig), colors::Reset);

  int TotalMatches = 0;

  // Iterate through collected results
  for (const auto &TaskRes : AllResults) {
    for (const auto &Result : TaskRes.Results) {
      fmt::print("{}{}✔ {}{}\n", colors::Bold, colors::Blue, Result.FileName,
                 colors::Reset);
      for (const auto &Match : Result.Matches) {
        fmt::print("  {}└─ {}{}: {}{}{}{}\n", colors::Grey, colors::Yellow,
                   Match.Line, colors::Reset, colors::Green, Match.FunctionName,
                   colors::Reset, Match.SignatureStr);
        TotalMatches++;
      }
    }

    for (const auto &File : TaskRes.Failures) {
      fmt::print("{}{}✖ Warning: {}{}Failed to parse {}\n", colors::Bold,
                 colors::Yellow, colors::Reset, File);
    }
  }

  fmt::print("\nMatches found: {}\n", TotalMatches);

  return 0;
}
