#include <iostream>
#include <string>
#include <vector>

// Simple arithmetic functions
int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

// String operations
std::string greet(const std::string &name) {
    return "Hello, " + name + "!";
}

// Pointer operations
void increment(int *value) {
    if (value) {
        (*value)++;
    }
}

void process(void *data, int size) {
    if (data && size > 0) {
        // Process raw data buffer
        char *bytes = static_cast<char*>(data);
        for (int i = 0; i < size; i++) {
            bytes[i] = bytes[i] ^ 0xFF; // Simple XOR transformation
        }
    }
}

// String getters
const char *getMessage() {
    return "System ready";
}

char *get_string(void) {
    return (char *)"Hello World";
}

// Vector operations
std::vector<int> doubleElements(const std::vector<int> &input) {
    std::vector<int> result;
    for (int val : input) {
        result.push_back(val * 2);
    }
    return result;
}

void printAll(const std::vector<std::string> &messages) {
    for (const auto &msg : messages) {
        std::cout << msg << std::endl;
    }
}

// Callback operations
void runCallback(void (*callback)(int)) {
    if (callback) {
        callback(42);
    }
}

// Complex data processing
bool processData(const std::string &label, void *data, size_t size) {
    if (label.empty() || !data || size == 0) {
        return false;
    }
    std::cout << "Processing " << size << " bytes with label: " << label << std::endl;
    return true;
}
