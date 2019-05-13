#pragma once

#include <cstddef>
#include <sys/mman.h>

namespace sdfjit::machinecode {

struct Machine_Code;
struct Executor {
  Machine_Code &mc;
  void *code{nullptr};
  size_t code_length{0};
  void *constants{nullptr};
  size_t constants_length{0};

  using Function_Type = void(void *xs, void *ys, void *zs, void *constants,
                             void *results);

  ~Executor() {
    if (code) {
      munmap(code, code_length);
    }
    if (constants) {
      munmap(constants, constants_length);
    }
  }

  void create();
  void call(void *xs, void *ys, void *zs, void *results) const;
};

} // namespace sdfjit::machinecode
