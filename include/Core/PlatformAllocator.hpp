#pragma once

#include "Assert.hpp"
#include "Logging.hpp"

#include <cstddef>
#include <cstdlib>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace memory::core {

/*
 NOTE:
 Thin wrapper over the OS virtual-memory API. Intended for a small number
 of large, whole page allocations, not for general-purpose small
 allocations.
*/
class PlatformAllocator {
public:
  // OS page size, queried once and cached.
  [[nodiscard]] static std::size_t PageSize() noexcept {
    static const std::size_t size = QueryPageSize();
    return size;
  }

  // Rounds `size` up to the next multiple of the platform page size.
  [[nodiscard]] static std::size_t AlignToPage(std::size_t size) noexcept {
    std::size_t page = PageSize();
    return (size + page - 1) & ~(page - 1);
  }

  // Allocates a minimum of bytes of PageSize()
  [[nodiscard]] static void *Allocate(std::size_t size) noexcept {
    if (size == 0)
      return nullptr;

    std::size_t alignedSize = AlignToPage(size);

#if defined(_WIN32)
    void *ptr = VirtualAlloc(nullptr, alignedSize, MEM_RESERVE | MEM_COMMIT,
                             PAGE_READWRITE);
#elif defined(__linux__) || defined(__unix__)
    void *ptr = mmap(nullptr, alignedSize, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr == MAP_FAILED)
      ptr = nullptr;
#else
    void *ptr = std::malloc(alignedSize);
#endif

    if (!ptr)
      LOG_ERROR(
          "PlatformAllocator::Allocate failed for {} bytes (aligned to {})",
          size, alignedSize);

    return ptr;
  }

  // `size` must be the *original* size passed to the matching Allocate()
  // call — it is re-aligned internally to recompute what was actually
  // committed. Passing a different size than what was allocated is
  // undefined behavior at the OS level (partial unmap on POSIX).
  static void Deallocate(void *ptr, std::size_t size) noexcept {
    if (!ptr)
      return;

    std::size_t alignedSize = AlignToPage(size);

#if defined(_WIN32)
    (void)alignedSize; // VirtualFree(MEM_RELEASE) requires dwSize == 0.
    BOOL ok = VirtualFree(ptr, 0, MEM_RELEASE);
    CORE_VERIFY(ok != 0, "VirtualFree failed to release memory");
#elif defined(__linux__) || defined(__unix__)
    int result = munmap(ptr, alignedSize);
    CORE_VERIFY(result == 0, "munmap failed to release memory");
#else
    std::free(ptr);
#endif
  }

private:
  [[nodiscard]] static std::size_t QueryPageSize() noexcept {
#if defined(_WIN32)
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return static_cast<std::size_t>(info.dwPageSize);
#elif defined(__linux__) || defined(__unix__)
    long result = sysconf(_SC_PAGESIZE);
    return result > 0 ? static_cast<std::size_t>(result) : 4096;
#else
    return 4096;
#endif
  }
};

} // namespace memory::core
