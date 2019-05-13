#include "bits.h"

namespace sdfjit::util {

union f2b_conv {
  float f;
  uint32_t i;
};

uint32_t float_to_bits(float f) {
  f2b_conv conv;
  conv.f = f;
  return conv.i;
}

float bits_to_float(uint32_t i) {
  f2b_conv conv;
  conv.i = i;
  return conv.f;
}

} // namespace sdfjit::util
