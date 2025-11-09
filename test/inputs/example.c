#include <iostream>
#include <string>
#include <vector>

int add([[maybe_unused]] int a, [[maybe_unused]] int b) { return 0; }

std::string greet([[maybe_unused]] const std::string &name) { return {}; }

void increment([[maybe_unused]] int *value) {}

void process([[maybe_unused]] void *data, [[maybe_unused]] int size) {}

const char *getMessage() { return "test"; }

std::vector<int>
doubleElements([[maybe_unused]] const std::vector<int> &input) {
  return {};
}

void printAll([[maybe_unused]] const std::vector<std::string> &messages) {}

void runCallback([[maybe_unused]] void (*callback)(int)) {}

bool processData([[maybe_unused]] const std::string &label,
                 [[maybe_unused]] void *data, [[maybe_unused]] size_t size) {
  return false;
}
