#include "raytracer.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <iomanip>

#include "bytecode/bytecode.h"
#include "machinecode/opt.h"
#include "util/macros.h"

namespace sdfjit::raytracer {

Raytracer Raytracer::from_ast(sdfjit::ast::Ast &ast) {
  auto bc = bytecode::Bytecode::from_ast(ast);
  auto mc = machinecode::Machine_Code::from_bytecode(bc);
  mc.resolve_immediates();
  mc.allocate_registers();
  mc.add_prologue_and_epilogue();
  machinecode::optimize(mc);
  Raytracer rt{{std::move(mc)}};
  rt.exec.create();
  return rt;
}

bool Raytracer::one_round(size_t width, size_t height, float *__restrict xs,
                          float *__restrict ys, float *__restrict zs,
                          float *__restrict dxs, float *__restrict dys,
                          float *__restrict dzs,
                          float *__restrict distances) const {
  const auto count = width * height;

  // get distances
  for (size_t offset = 0; offset < count; offset += 8) {
    exec.call(&xs[offset], &ys[offset], &zs[offset], &distances[offset]);
  }

  // update positions:

  bool not_done = false;

#if 0
  // unvectorized:
  for (size_t offset = 0; offset < count; offset++) {
    if (0 < distances[offset] && distances[offset] < MAX_DIST) {
      not_done = true;
      float dist = distances[offset] + .1f;
      xs[offset] += dxs[offset] * dist;
      ys[offset] += dys[offset] * dist;
      zs[offset] += dzs[offset] * dist;
    }
  }

#else

  // vectorized:
  // this whole thing would be much easier if we could use the AVX512
  // writemasked operations, but this project is all AVX256 for now
  // upper/lower bounds on all lanes:
  auto lower_bound = _mm256_setzero_ps();
  auto upper_bound = _mm256_set1_ps(MAX_DIST);
  auto epsilon = _mm256_set1_ps(0.1f);

  auto advance = [](float *__restrict ps, float *__restrict dps, __m256 dist,
                    __m256 op_mask, size_t off) {
    const auto p = _mm256_load_ps(&ps[off]);
    const auto dp = _mm256_load_ps(&dps[off]);
    const auto retained_p = _mm256_andnot_ps(op_mask, p);
    const auto new_p = _mm256_fmadd_ps(dist, dp, p);
    const auto changed_p = _mm256_and_ps(op_mask, new_p);
    const auto result_p = _mm256_or_ps(changed_p, retained_p);
    _mm256_store_ps(&ps[off], result_p);
  };

  for (size_t offset = 0; offset < count; offset += 8) {
    auto dist = _mm256_load_ps(&distances[offset]);

    auto low_mask = _mm256_cmp_ps(lower_bound, dist, _CMP_LT_OS);
    auto high_mask = _mm256_cmp_ps(dist, upper_bound, _CMP_LT_OS);
    auto op_mask = _mm256_and_ps(low_mask, high_mask);

    if (UNLIKELY(!_mm256_testz_ps(op_mask, op_mask))) {
      not_done = true;
    } else {
      // doing this operation would be a waste anyways, dont spend cycles on
      // it.
      // XXX: is this better than just letting it do the op or not?
      continue;
    }

    dist = _mm256_add_ps(dist, epsilon);

    advance(xs, dxs, dist, op_mask, offset);
    advance(ys, dys, dist, op_mask, offset);
    advance(zs, dzs, dist, op_mask, offset);
  }

#endif

  return not_done;
}

void Raytracer::trace_image(float px, float py, float pz, float hx, float hy,
                            float hz, size_t width, size_t height,
                            uint32_t *screen) const {
  // our raytracing setup right now is that we send out a ray for each pixel.
  // our screen is 3-component _RGB (top byte of the pixel is always empty)
  const auto count = width * height;
  // current ray positions:
  float *xs = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  float *ys = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  float *zs = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  // distance buffer:
  float *distances = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  // normalized directions rays are moving in:
  float *dxs = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  float *dys = (float *)aligned_alloc(256 / 8, count * sizeof(float));
  float *dzs = (float *)aligned_alloc(256 / 8, count * sizeof(float));

  const float invWidth = 1 / (float)width, invHeight = 1 / (float)height;
  const float fov = 45, aspectratio = width / (float)height;
  const float angle = tan(M_PI * 0.5 * fov / 180.0);

  // TODO: base around this heading instead of fixed
  (void)hx;
  (void)hy;
  (void)hz;

  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      float xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio;
      float yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle;
      float zz = -1;

      // normalize xx, yy, zz:
      float len = sqrt(xx * xx + yy * yy + zz * zz);
      xx /= len;
      yy /= len;
      zz /= len;

      size_t offset = y * width + x;

      // setup rey directions:
      dxs[offset] = xx;
      dys[offset] = yy;
      dzs[offset] = zz;

      // setup initial ray positions as being at the camera:
      xs[offset] = px;
      ys[offset] = py;
      zs[offset] = pz;
    }
  }

  while (one_round(width, height, xs, ys, zs, dxs, dys, dzs, distances))
    ;

  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      size_t offset = y * width + x;

      if (distances[offset] <= 0) {
        screen[offset] = 0x00ffffff;
      } else {
        screen[offset] = 0x00000000;
      }
    }
  }

  free(xs);
  free(ys);
  free(zs);
  free(distances);
  free(dxs);
  free(dys);
  free(dzs);
}

} // namespace sdfjit::raytracer
