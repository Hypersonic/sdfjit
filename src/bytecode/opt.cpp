#include "opt.h"

#include "bytecode.h"
#include "passes/cse.h"

namespace sdfjit::bytecode {

void optimize(Bytecode &bc) {
  passes::common_subexpression_elimination(bc);
  return;
}

} // namespace sdfjit::bytecode
