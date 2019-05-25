#pragma once

namespace sdfjit::bytecode {
struct Bytecode;
}

namespace sdfjit::bytecode::passes {

void constant_fold(Bytecode &bc);

} // namespace sdfjit::bytecode::passes
