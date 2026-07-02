#pragma once

#include "../Core/Assert.hpp"
#include "../Core/Logging.hpp"
#include "../Core/PlatformAllocator.hpp"
#include "../Core/Utils.hpp"

#include <cstddef>
#include <new>

namespace memory {

/*
  Owns a single contiguous block of memory (reserved via PlatformAllocator)
  and serves allocations by advancing a pointer through it.

 NOTE:
  This is the shared resource that ArenaAllocator<T> uses (see
 ArenaAllocator.hpp for why the split exists). Individual deallocation is not
 supported — see ArenaAllocator::deallocate. The whole block is reclaimed at
 once via Reset() (just rewinds the pointer, does not run any
 destructors) or when the Arena itself is destroyed.

 WARNING:
  Not copyable, not movable: every ArenaAllocator<T> handle pointing at this
 arena holds a raw Arena*, so relocating it would invalidate all of
 them. Construct one where its lifetime clearly dominates every
 container/handle that uses it. Not thread-safe.
*/

class Arena {
public:
  explicit Arena(std::size_t capacity)
      : m_Capacity(core::PlatformAllocator::AlignToPage(capacity)) {
    m_Begin =
        static_cast<std::byte *>(core::PlatformAllocator::Allocate(capacity));
    CORE_VERIFY(m_Begin != nullptr, "Arena failed to reserve backing memory");
  }

  ~Arena() {
    if (m_Begin)
      core::PlatformAllocator::Deallocate(m_Begin, m_Capacity);
  }

  Arena(const Arena &) = delete;
  Arena &operator=(const Arena &) = delete;
  Arena(Arena &&) = delete;
  Arena &operator=(Arena &&) = delete;

  /*
   Returns a block of at least `size` bytes aligned to `alignment`
   (must be a power of two). Throws std::bad_alloc if the arena doesn't
   have room — this is a normal, recoverable condition (you sized the
   arena too small for the workload), not a programmer-error abort like
   the checks in PlatformAllocator. Callers relying on standard container
   exception guarantees need allocate() to throw rather than abort here.
  */
  [[nodiscard]] void *Allocate(std::size_t size, std::size_t alignment) {
    CORE_ASSERT(core::detail::IsPowOfTwo(alignment),
                "alignment must be a power of two");

    auto offset = core::detail::AlignUp(m_Offset, alignment) + size;
    if (offset > m_Capacity) {
      LOG_WARN("Arena exhausted: requested {} bytes (align {}), {} "
               "bytes remaining",
               size, alignment, BytesRemaining());
      throw std::bad_alloc{};
    }

    m_Offset = offset;
    return m_Begin + offset - size;
  }

  /*
   Rewinds the arena to the beginning, making its full capacity available
   again. Does NOT run destructors on anything previously allocated from
   it — callers must have already destroyed those objects.
  */
  void Reset() noexcept { m_Offset = 0; }

  [[nodiscard]] std::size_t Capacity() const noexcept { return m_Capacity; }

  [[nodiscard]] std::size_t BytesUsed() const noexcept { return m_Offset; }

  [[nodiscard]] std::size_t BytesRemaining() const noexcept {
    return m_Capacity - m_Offset;
  }

private:
  std::size_t m_Capacity{}, m_Offset{};
  std::byte *m_Begin{};
};
} // namespace memory
