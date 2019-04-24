#pragma once

#include "ast.h"

namespace sdfjit::ast::opt {

void combine_identical_nodes(Ast &ast);

}
