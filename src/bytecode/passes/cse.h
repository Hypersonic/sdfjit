#pragma once

namespace sdfjit::bytecode {
struct Bytecode;
}

namespace sdfjit::bytecode::passes {

void common_subexpression_elimination(Bytecode &bc);

} // namespace sdfjit::bytecode::passes
