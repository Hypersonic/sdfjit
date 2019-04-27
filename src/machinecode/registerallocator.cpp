#include "registerallocator.h"

#include <vector>

#include "machinecode.h"

namespace sdfjit::machinecode {

void Linear_Scan_Register_Allocator::allocate(Machine_Code &mc) {
  compute_live_intervals(mc);
}

void Linear_Scan_Register_Allocator::compute_live_intervals(Machine_Code &mc) {
  for (size_t i = 0; i < mc.instructions.size(); i++) {
    auto &insn = mc.instructions[i];
    for (auto &reg : insn.registers) {
      if (!reg.is_virtual())
        continue;

      if (live_intervals.contains(reg)) {
        live_intervals.at(reg).last = i;
      } else {
        auto &interval = live_intervals[reg];
        interval.first = interval.last = i;
      }
    }
  }
}

} // namespace sdfjit::machinecode
