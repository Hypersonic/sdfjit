#include "opt.h"

#include "bytecode.h"
#include "passes/constant_fold.h"
#include "passes/cse.h"
#include "passes/unused_value_elimination.h"

namespace sdfjit::bytecode {

void optimize(Bytecode &bc) {
  passes::common_subexpression_elimination(bc);
  passes::constant_fold(bc);
  passes::unused_value_elimination(bc);
}

} // namespace sdfjit::bytecode
