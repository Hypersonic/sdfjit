#include "opt.h"

#include "passes/movelimination.h"

namespace sdfjit::machinecode {

void optimize(Machine_Code &mc) {
  (void)mc;
  passes::peephole_eliminate_movs(mc);
  // TODO: eliminate nops
}

} // namespace sdfjit::machinecode
