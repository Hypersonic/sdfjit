#include "movelimination.h"

namespace sdfjit::machinecode::passes {

void peephole_eliminate_movs(Machine_Code &mc) {
  for (size_t i = 1; i < mc.instructions.size(); i++) {
    auto &insn1 = mc.instructions[i - 1];
    auto &insn2 = mc.instructions[i];

    if (insn1.op != Op::vmovaps || insn2.op != Op::vmovaps) {
      continue;
    }

    auto insn1_dst = insn1.registers.at(0);
    auto insn2_src = insn2.registers.at(1);
    if (insn1_dst.is_memory() && insn1_dst == insn2_src) {
      // sadly, we can't get rid of the store (insn1), because later
      // instructions might depend on it.
      // Maybe it's worth doing a pass here to see if we can kill it, but i'm
      // worried about the O(n^2) nature of that

      insn2.registers[1] = insn1.registers[1];

      // if we'd just be moving a register over itself, kill the mov
      if (insn2.registers[0] == insn2.registers[1]) {
        insn2.convert_to_nop();
      }
    }
  }
}

} // namespace sdfjit::machinecode::passes
