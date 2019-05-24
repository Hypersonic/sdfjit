#pragma once

#include <cstddef>
#include <sys/mman.h>

#include "machinecode/machinecode.h"

namespace sdfjit::machinecode {

struct Machine_Code;
struct Executor {
  Machine_Code mc;
  void *code{nullptr};
  size_t code_length{0};
  void *constants{nullptr};
  size_t constants_length{0};

  using Function_Type = void(void *xs, void *ys, void *zs, void *constants,
                             void *results);

  ~Executor() {
    const auto unmap = [](void *ptr, size_t length) {
    // in release builds we just unmap.
    // However, it it useful to not have allocations ever be reused, so we can
    // always differentiate which jit page we're talking about, for example when
    // profiling. When this is the case we just mprotect them to nothing and
    // MADV_DONTNEED them. This still takes up a page table entry, but that's
    // fine.
#ifdef RELEASE
      munmap(ptr, length);
#else
      mprotect(ptr, length, 0);
      madvise(ptr, length, MADV_DONTNEED);
#endif
    };

    if (code) {
      unmap(code, code_length);
    }
    if (constants) {
      unmap(constants, constants_length);
    }
  }

  void create();
  void call(void *xs, void *ys, void *zs, void *results) const;
};

} // namespace sdfjit::machinecode
