#pragma once

#include <cstddef>
#include <cstdint>

namespace memory::core {
namespace detail {

[[nodiscard]] constexpr std::size_t AlignUp(std::size_t val,
                                            std::size_t alignment) noexcept {
  return (val + alignment - 1) & ~(alignment - 1);
}

[[nodiscard]] constexpr std::byte *AlignUp(std::byte *ptr,
                                           std::size_t alignment) noexcept {
  auto addr = reinterpret_cast<std::uintptr_t>(ptr);
  auto aligned = (addr + alignment - 1) & ~(alignment - 1);
  return reinterpret_cast<std::byte *>(aligned);
}

[[nodiscard]] constexpr bool IsPowOfTwo(std::size_t val) noexcept {
  return (val & (val - 1)) == 0;
}
} // namespace detail
} // namespace memory::core
