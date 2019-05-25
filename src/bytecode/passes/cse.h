#pragma once

#include "bytecode/bytecode.h"

namespace sdfjit::bytecode::passes {

void common_subexpression_elimination(Bytecode &bc);

}
