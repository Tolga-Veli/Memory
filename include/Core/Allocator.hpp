#pragma once

#include <concepts>
#include <cstddef>

namespace memory::core {

template <class T>
concept Allocator =
    requires(T allocator, std::size_t size, std::size_t alignment, T *ptr) {
      { allocator.allocate(size, alignment) } -> std::same_as<T *>;
      { allocator.deallocate(ptr, size, alignment) } -> std::same_as<void>;
    };
} // namespace memory::core
