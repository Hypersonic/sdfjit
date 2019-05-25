#include "compare.h"

#include <cmath>

namespace sdfjit::util {

bool floats_equal(const float a, const float b) {
  return fabs(a - b) < 0.00001f;
}

} // namespace sdfjit::util
