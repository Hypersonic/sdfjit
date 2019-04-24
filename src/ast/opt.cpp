#include "opt.h"

#include "combine_identical_nodes.h"

namespace sdfjit::ast::opt {

void optimize(Ast &ast) { combine_identical_nodes(ast); }

} // namespace sdfjit::ast::opt
