#include <string>
#include <vector>
#include <iostream>

int add(int a, int b) {
    (void)a;
    (void)b;
    return 0;
}

std::string greet(const std::string& name) {
    (void)name;
    return {};
}

void increment(int* value) {
    (void)value;
}

void process(void* data, int size) {
    (void)data;
    (void)size;
}

const char* getMessage() {
    return "test";
}

std::vector<int> doubleElements(const std::vector<int>& input) {
    (void)input;
    return {};
}

void printAll(const std::vector<std::string>& messages) {
    (void)messages;
}

void runCallback(void (*callback)(int)) {
    (void)callback;
}

bool processData(const std::string& label, void* data, size_t size) {
    (void)label;
    (void)data;
    (void)size;
    return false;
}
