#pragma once

namespace sdfjit::bytecode {
struct Bytecode;
}

namespace sdfjit::bytecode::passes {

void unused_value_elimination(Bytecode &bc);

} // namespace sdfjit::bytecode::passes
