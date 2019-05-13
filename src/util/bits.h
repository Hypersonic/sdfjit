#pragma once

#include <stdint.h>

namespace sdfjit::util {

uint32_t float_to_bits(float f);
float bits_to_float(uint32_t i);

} // namespace sdfjit::util
