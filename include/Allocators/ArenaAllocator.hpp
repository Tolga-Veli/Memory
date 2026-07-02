#pragma once

#include "Arena.hpp"

#include <cstddef>
#include <limits>
#include <type_traits>

namespace memory {

/*
 Adapts an Arena into a type usable as the Allocator template parameter
 of std::vector, std::basic_string, std::list, or your own containers.

 This is deliberately a thin, cheap-to-copy handle — it holds only a
 pointer to a Arena, never memory itself. That's not a style choice,
 it's required: STL containers copy and rebind their allocator internally
 (e.g. std::list<T, Alloc> constructing an Alloc<Node> from your
 Alloc<T>), and the standard Allocator requirements assume that's cheap.
 All real state lives in the Arena, which you own and must keep alive
 for at least as long as every container using it:

 NOTE:
   Arena arena(1 << 20);                        // 1 MiB, outer scope
   std::vector<int, ArenaAllocator<int>> v{ArenaAllocator<int>(arena)};

 NOTE:
 deallocate() is intentionally a no-op — arena allocators cannot reclaim
 individual objects, only the whole arena at once via Arena::Reset().
 Good fit for scoped/per-frame  workloads; a poor fit for long-lived containers
 with heavy churn (frequent allocations and deallocation).
*/
template <typename T> class ArenaAllocator {
public:
  using value_type = T;

  // Rebinding produces a new handle to the same arena, so propagating
  // the allocator on copy/move/swap is always correct and cheap.
  using propagate_on_container_copy_assignment = std::true_type;
  using propagate_on_container_move_assignment = std::true_type;
  using propagate_on_container_swap = std::true_type;

  // Two ArenaAllocators over different arenas are not interchangeable,
  // so this must NOT be true_type even though the handle itself is trivial.
  using is_always_equal = std::false_type;

  explicit ArenaAllocator(Arena &arena) noexcept : m_Arena(&arena) {}

  // Converting constructor: required so containers can rebind
  // ArenaAllocator<T> to ArenaAllocator<U> (e.g. a node type in std::list)
  // while pointing at the same underlying arena.
  template <typename U>
  ArenaAllocator(const ArenaAllocator<U> &other) noexcept
      : m_Arena(other.Arena()) {}

  [[nodiscard]] T *allocate(std::size_t n) {
    CORE_VERIFY(static_cast<std::uint64_t>(n) * sizeof(T) <
                    std::numeric_limits<std::size_t>::max(),
                "Invalid allocation size");

    void *ptr = m_Arena->Allocate(n * sizeof(T), alignof(T));
    return static_cast<T *>(ptr);
  }

  void deallocate(T *, std::size_t) const noexcept {}

  [[nodiscard]] Arena *GetArena() const noexcept { return m_Arena; }

  template <typename U>
  [[nodiscard]] bool operator==(const ArenaAllocator<U> &other) const noexcept {
    return m_Arena == other.Arena();
  }

private:
  Arena *m_Arena{};
};

} // namespace memory
