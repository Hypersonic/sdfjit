#pragma once

#include <stdint.h>
#include <vector>

namespace sdfjit::machinecode {

struct Stack_Info {
  uint32_t current_offset{0};

  uint32_t add_slot(size_t slot_size) {
    auto ret = current_offset;
    current_offset += slot_size;
    return ret;
  }
};

} // namespace sdfjit::machinecode
