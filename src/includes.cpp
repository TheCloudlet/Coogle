// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// System include path detection by invoking the system C++ compiler
// and parsing its output.

#include "coogle/includes.h"

#include <cstdio>
#include <fmt/core.h>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

constexpr std::size_t BufferSize = 512;

// Windows is not supported for now
std::vector<std::string> detectSystemIncludePaths() {
  std::vector<std::string> IncludePaths;
  FILE *Pipe = popen("clang -E -x c /dev/null -v 2>&1", "r");
  if (!Pipe) {
    std::cerr << "Failed to run clang for include path detection.\n";
    return IncludePaths;
  }

  char Buffer[BufferSize];
  bool InsideSearch = false;

  constexpr char IncludeStart[] = "#include <...> search starts here:";
  constexpr char IncludeEnd[] = "End of search list.";

  while (fgets(Buffer, sizeof(Buffer), Pipe)) {
    std::string Line = Buffer;

    if (Line.find(IncludeStart) != std::string::npos) {
      InsideSearch = true;
      continue;
    }

    if (InsideSearch && Line.find(IncludeEnd) != std::string::npos) {
      break;
    }

    if (InsideSearch) { // Extract path
      std::string Path =
          std::regex_replace(Line, std::regex("^\\s+|\\s+$"), "");
      if (!Path.empty()) {
        IncludePaths.push_back("-I" + Path);
      }
    }
  }

  pclose(Pipe);
  return IncludePaths;
}
