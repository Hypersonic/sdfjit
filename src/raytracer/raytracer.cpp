#include "raytracer.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <iomanip>
#include <memory>
#include <pthread.h>

#include "bytecode/bytecode.h"
#include "bytecode/opt.h"
#include "machinecode/opt.h"
#include "util/macros.h"

namespace sdfjit::raytracer {

Raytracer Raytracer::from_ast(sdfjit::ast::Ast &ast) {
  auto bc = bytecode::Bytecode::from_ast(ast);
  bytecode::optimize(bc);
  auto mc = machinecode::Machine_Code::from_bytecode(bc);
  mc.resolve_immediates();
  mc.allocate_registers();
  mc.add_prologue_and_epilogue();
  machinecode::optimize(mc);
  Raytracer rt{{std::move(mc)}};
  rt.exec.create();
  return rt;
}

bool Raytracer::one_round(size_t count, float *__restrict xs,
                          float *__restrict ys, float *__restrict zs,
                          float *__restrict dxs, float *__restrict dys,
                          float *__restrict dzs, float *__restrict distances,
                          float *__restrict materials) const {
  // get distances
  for (size_t offset = 0; offset < count; offset += 8) {
    exec.call(&xs[offset], &ys[offset], &zs[offset], &distances[offset],
              &materials[offset]);
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

struct Trace_Thread_Arg {
  const Raytracer *rt;
  size_t count;
  float *xs;
  float *ys;
  float *zs;
  float *dxs;
  float *dys;
  float *dzs;
  float *distances;
  float *materials;
};

void *trace_thread(Trace_Thread_Arg *arg) {
  while (arg->rt->one_round(arg->count, arg->xs, arg->ys, arg->zs, arg->dxs,
                            arg->dys, arg->dzs, arg->distances, arg->materials))
    ;
  return NULL;
}

void Raytracer::trace_image(float px, float py, float pz, float hx, float hy,
                            float hz, size_t width, size_t height,
                            uint32_t *screen) const {
  // our raytracing setup right now is that we send out a ray for each pixel.
  // our screen is 3-component _RGB (top byte of the pixel is always empty)
  const auto count = width * height;
  const auto alignment = 256 / 8;
  const auto num_threads = 16; // XXX: should get number of cpus dynamically

  auto make_buffer = [&](size_t size) -> std::unique_ptr<float[]> {
    auto alloc = (float *)aligned_alloc(alignment, size * sizeof(float));
    return std::move(std::unique_ptr<float[]>(alloc));
  };

  auto make_count_buffer = [&]() { return make_buffer(count); };

  // current ray positions:
  auto xs = make_count_buffer();
  auto ys = make_count_buffer();
  auto zs = make_count_buffer();
  // distance & material buffers:
  auto distances = make_count_buffer();
  auto materials = make_count_buffer();
  // for the reflections (XXX: make this an array so we can do multiple
  // reflections)
  auto reflected_materials = make_count_buffer();
  // normalized directions rays are moving in:
  auto dxs = make_count_buffer();
  auto dys = make_count_buffer();
  auto dzs = make_count_buffer();
  // buffers for doing normal estimation, we need to compute 2 positions on
  // either side of the original position in each axis to estimate the normal
  auto normal_estimation_xs = make_buffer(2 * count);
  auto normal_estimation_ys = make_buffer(2 * count);
  auto normal_estimation_zs = make_buffer(2 * count);

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

      // setup ray directions:
      dxs[offset] = xx;
      dys[offset] = yy;
      dzs[offset] = zz;

      // setup initial ray positions as being at the camera:
      xs[offset] = px;
      ys[offset] = py;
      zs[offset] = pz;
    }
  }

  // pass 1: find geometry collisions
  std::vector<Trace_Thread_Arg> thread_args{};
  std::vector<pthread_t> threads{};

  // we need to avoid reallocating since we're passing pointers into this vector
  thread_args.reserve(num_threads);
  auto run_length = count / num_threads;
  for (size_t i = 0; i < num_threads; i++) {
    auto offset = i * run_length;
    thread_args.push_back(
        Trace_Thread_Arg{this, run_length, xs.get() + offset, ys.get() + offset,
                         zs.get() + offset, dxs.get() + offset,
                         dys.get() + offset, dzs.get() + offset,
                         distances.get() + offset, materials.get() + offset});

    pthread_t thread;
    pthread_create(&thread, NULL, (void *(*)(void *))trace_thread,
                   &thread_args.back());
    threads.push_back(thread);
  }

  for (auto thread : threads) {
    pthread_join(thread, NULL);
  }

  // fill in screen with initial colors
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      size_t offset = y * width + x;

      if (distances[offset] <= 0) {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        // XXX: move this to some kind of "material lookup"
        if (util::floats_equal(materials[offset], 1.0f)) {
          r = 0xff;
        }
        if (util::floats_equal(materials[offset], 2.0f)) {
          g = 0xff;
        }
        if (util::floats_equal(materials[offset], 3.0f)) {
          b = 0xff;
        }

        screen[offset] = (r << 16) | (g << 8) | (b);
      } else {
        screen[offset] = 0x00000000;
      }
    }
  }

  // pass 2: calculate normals:
  /*
  static constexpr float normal_epsilon = .001;
  for (size_t i = 0; i < count; i++) {
    //.x = sdf(translate_x(pos, eps)) - sdf(translate_x(pos, -eps)),
    //.y = sdf(translate_y(pos, eps)) - sdf(translate_y(pos, -eps)),
    //.z = sdf(translate_z(pos, eps)) - sdf(translate_z(pos, -eps)),

    // XXX: ugh this shit is wrong, we need to have 6 full component vectors I
    // think UGH... actually maybe we can do it with these and the modified
    // collision points... TODO when i'm more awake
    normal_estimation_xs[(i * 2) + 0] = xs[i] + normal_epsilon;
    normal_estimation_xs[(i * 2) + 1] = xs[i] - normal_epsilon;
    normal_estimation_ys[(i * 2) + 0] = ys[i] + normal_epsilon;
    normal_estimation_ys[(i * 2) + 1] = ys[i] - normal_epsilon;
    normal_estimation_zs[(i * 2) + 0] = zs[i] + normal_epsilon;
    normal_estimation_zs[(i * 2) + 1] = zs[i] - normal_epsilon;
  }
  */
}

} // namespace sdfjit::raytracer
