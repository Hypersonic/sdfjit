#pragma once

#include <cstddef>

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

  void create();
  void call(void *xs, void *ys, void *zs, void *results);
};

} // namespace sdfjit::machinecode
