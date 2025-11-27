// Test file for type alias handling
#include <cstdint>
#include <string>

// Type aliases using "using"
using MyInt = int;
using StringPtr = std::string *;
using ConstCharPtr = const char *;

// Typedefs
typedef int Integer;
typedef void (*Callback)(int);

// Functions using type aliases as return types
MyInt getMyInt() {
    return 42;
}

Integer getInteger() {
    return 100;
}

StringPtr getStringPtr() {
    return nullptr;
}

// Functions using type aliases as parameters
void processMyInt(MyInt value) {
    (void)value;
}

void processInteger(Integer value) {
    (void)value;
}

void processStringPtr(StringPtr ptr) {
    (void)ptr;
}

void processConstCharPtr(ConstCharPtr ptr) {
    (void)ptr;
}

// Function using callback typedef
void runCallback(Callback cb) {
    if (cb) cb(42);
}

// Standard library type aliases
void processSize(std::size_t size) {
    (void)size;
}

void processInt32(std::int32_t value) {
    (void)value;
}
