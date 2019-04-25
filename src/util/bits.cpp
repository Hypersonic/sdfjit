#include "bits.h"

namespace sdfjit::util {

uint32_t float_to_bits(float f) {
  union {
    float f;
    uint32_t i;
  } conv;

  conv.f = f;
  return conv.i;
}

} // namespace sdfjit::util
