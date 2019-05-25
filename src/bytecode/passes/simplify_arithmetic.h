#pragma once

namespace sdfjit::bytecode {
struct Bytecode;
}

namespace sdfjit::bytecode::passes {

void simplify_arithmetic(Bytecode &bc);

} // namespace sdfjit::bytecode::passes
