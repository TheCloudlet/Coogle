// Copyright 2025 Yi-Ping Pan (Cloudlet)
//
// Memory arena allocator and C++17-compatible span for zero-allocation
// string management.

#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

namespace coogle {

// Simple C++17-compatible span implementation.
// A non-owning view over a contiguous sequence of objects.
template <typename T> class span {
  T *Data_;
  std::size_t Size_;

public:
  constexpr span() noexcept : Data_(nullptr), Size_(0) {}
  constexpr span(T *Data, std::size_t Size) noexcept
      : Data_(Data), Size_(Size) {}

  template <std::size_t N>
  constexpr span(T (&Array)[N]) noexcept : Data_(Array), Size_(N) {}

  constexpr T *data() const noexcept { return Data_; }
  constexpr std::size_t size() const noexcept { return Size_; }
  constexpr bool empty() const noexcept { return Size_ == 0; }

  constexpr T &operator[](std::size_t Idx) const noexcept { return Data_[Idx]; }

  constexpr T *begin() const noexcept { return Data_; }
  constexpr T *end() const noexcept { return Data_ + Size_; }

  constexpr span<T>
  subspan(std::size_t Offset,
          std::size_t Count = static_cast<std::size_t>(-1)) const noexcept {
    if (Count == static_cast<std::size_t>(-1)) {
      Count = Size_ - Offset;
    }
    return span<T>(Data_ + Offset, Count);
  }
};

// Arena allocator for zero-allocation string storage.
// All allocated strings remain valid for the arena's lifetime.
//
// This class provides a bump allocator for strings, storing all data
// in a single contiguous buffer for cache-friendly access.
class StringArena {
  std::vector<char> Buffer_;

public:
  StringArena() { Buffer_.reserve(4096); }

  // Allocates and copies a string into the arena.
  // Returns a string_view pointing to the allocated memory.
  // The returned string_view remains valid for the arena's lifetime.
  std::string_view intern(std::string_view Str) {
    const size_t Offset = Buffer_.size();
    Buffer_.insert(Buffer_.end(), Str.begin(), Str.end());
    Buffer_.push_back('\0'); // Null-terminate for C compatibility
    return std::string_view(Buffer_.data() + Offset, Str.size());
  }

  // Allocates space and returns writable buffer.
  // Useful for in-place transformations like normalizeType().
  span<char> allocate(size_t Size) {
    const size_t Offset = Buffer_.size();
    Buffer_.resize(Buffer_.size() + Size + 1); // +1 for null terminator
    return span<char>(Buffer_.data() + Offset, Size);
  }

  // Finalizes a span into a string_view with the actual used size.
  // Must be called after writing to a buffer from allocate().
  std::string_view finalize(span<char> Span, size_t ActualSize) {
    // Adjust buffer size to actual used size
    const size_t Offset = Span.data() - Buffer_.data();
    Buffer_.resize(Offset + ActualSize + 1);
    Buffer_[Offset + ActualSize] = '\0';
    return std::string_view(Span.data(), ActualSize);
  }

  // Clears all allocations (for reuse).
  void clear() { Buffer_.clear(); }

  // Gets current buffer size (for debugging/profiling).
  size_t size() const { return Buffer_.size(); }

  // Prevent copies (would invalidate string_views)
  StringArena(const StringArena &) = delete;
  StringArena &operator=(const StringArena &) = delete;

  // Allow moves
  StringArena(StringArena &&) noexcept = default;
  StringArena &operator=(StringArena &&) noexcept = default;
};

} // namespace coogle
