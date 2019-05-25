#include "opt.h"

#include "bytecode.h"
#include "passes/constant_fold.h"
#include "passes/cse.h"

namespace sdfjit::bytecode {

void optimize(Bytecode &bc) {
  passes::common_subexpression_elimination(bc);
  passes::constant_fold(bc);
  return;
}

} // namespace sdfjit::bytecode
