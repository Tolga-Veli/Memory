#pragma once

#include "../Core/Allocator.hpp"
#include "../Core/Assert.hpp"

#include <memory>
#include <utility>

namespace memory {

template <class T, core::Allocator Allocator_t> class Scoped {
public:
  constexpr Scoped() noexcept = default;
  constexpr Scoped(T *ptr, Allocator_t &allocator) noexcept
      : m_Ptr(ptr), m_Allocator(&allocator) {}
  ~Scoped() { Reset(); }

  Scoped(const Scoped &) = delete;
  Scoped &operator=(const Scoped &) = delete;

  constexpr Scoped(Scoped &&other) noexcept
      : m_Ptr(std::exchange(other.m_Ptr, nullptr),
              m_Allocator(std::exchange(other.m_Allocator, nullptr))) {}

  Scoped &operator=(Scoped &&other) noexcept {
    if (this == &other)
      return *this;

    Reset();

    m_Ptr = std::exchange(other.m_Ptr, nullptr);
    m_Allocator = std::exchange(other.m_Allocator, nullptr);

    return *this;
  }

  [[nodiscard]]
  constexpr T *Get() const noexcept {
    return m_Ptr;
  }

  [[nodiscard]]
  constexpr T &operator*() const noexcept {
    CORE_VERIFY(m_Ptr, "Dereferencing null Scoped.");
    return *m_Ptr;
  }

  [[nodiscard]]
  constexpr T *operator->() const noexcept {
    CORE_VERIFY(m_Ptr, "Dereferencing null Scoped.");
    return m_Ptr;
  }

  [[nodiscard]]
  constexpr explicit operator bool() const noexcept {
    return m_Ptr != nullptr;
  }

  constexpr T *Release() noexcept { return std::exchange(m_Ptr, nullptr); }

  void Reset() noexcept {
    if (!m_Ptr)
      return;

    CORE_VERIFY(m_Allocator, "Scoped contains a pointer but no allocator.");

    std::destroy_at(m_Ptr);

    m_Allocator->Deallocate(m_Ptr, sizeof(T), alignof(T));

    m_Ptr = nullptr;
  }

private:
  T *m_Ptr{};
  Allocator_t *m_Allocator{};
};

template <typename T, core::Allocator Allocator_t, typename... Args>
[[nodiscard]]
Scoped<T, Allocator_t> CreateScoped(Allocator_t &allocator, Args &&...args) {
  auto *mem = allocator.allocate(sizeof(T), alignof(T));
  CORE_VERIFY(mem, "Allocator returned nullptr.");

  return Scoped<T, Allocator_t>(::new (mem) T(std::forward<Args>(args)...),
                                allocator);
}

} // namespace memory
