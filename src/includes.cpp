// includes.cpp
#include "includes.h"

#include <cstdio>
#include <format>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

constexpr std::size_t BUFFER_SIZE = 512;

// Windows is not supported for now
std::vector<std::string> detectSystemIncludePaths() {
  std::vector<std::string> includePaths;
  FILE *pipe = popen("clang -E -x c /dev/null -v 2>&1", "r");
  if (!pipe) {
    std::cerr << "Failed to run clang for include path detection.\n";
    return includePaths;
  }

  char buffer[BUFFER_SIZE];
  bool insideSearch = false;

  constexpr char INCLUDE_START[] = "#include <...> search starts here:";
  constexpr char INCLUDE_END[] = "End of search list.";

  while (fgets(buffer, sizeof(buffer), pipe)) {
    std::string line = buffer;

    if (line.find(INCLUDE_START) != std::string::npos) {
      insideSearch = true;
      continue;
    }

    if (insideSearch && line.find(INCLUDE_END) != std::string::npos) {
      break;
    }

    if (insideSearch) { // Extract path
      std::string path =
          std::regex_replace(line, std::regex("^\\s+|\\s+$"), "");
      if (!path.empty()) {
        includePaths.push_back("-I" + path);
      }
    }
  }

  pclose(pipe);
  return includePaths;
}
