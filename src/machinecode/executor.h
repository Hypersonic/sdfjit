#pragma once

namespace sdfjit::machinecode {

struct Executor {
  Machine_Code &mc;

  using Function_Type = void(void *xs, void *ys, void *zs, void *constants,
                             void *results);

  void create();
};

} // namespace sdfjit::machinecode
