#pragma once

#include <stdint.h>
#include <vector>

namespace sdfjit::machinecode {

struct Stack_Info {
  size_t current_offset{0};

  size_t add_slot(size_t slot_size) {
    size_t ret = current_offset;
    current_offset += slot_size;
    return ret;
  }
};

} // namespace sdfjit::machinecode
