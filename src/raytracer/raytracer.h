#pragma once

#include "ast/ast.h"
#include "machinecode/executor.h"

namespace sdfjit::raytracer {

using machinecode::Executor;

struct Raytracer {
  Executor exec;
  static constexpr size_t MAX_DIST = 10000;

  static Raytracer from_ast(sdfjit::ast::Ast &ast);

  bool one_round(size_t width, size_t height, float *xs, float *ys, float *zs,
                 float *dxs, float *dys, float *dzs, float *distances) const;

  void trace_image(float x, float y, float z, float hx, float hy, float hz,
                   size_t width, size_t height, uint32_t *screen) const;
};

} // namespace sdfjit::raytracer
