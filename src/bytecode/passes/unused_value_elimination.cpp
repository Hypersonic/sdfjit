#include "unused_value_elimination.h"

#include <algorithm>

#include "bytecode/bytecode.h"

namespace sdfjit::bytecode::passes {

void unused_value_elimination(Bytecode &bc) {
  auto is_unused = [&bc](Node_Id id) -> bool {
    return std::none_of(
        bc.nodes.begin(), bc.nodes.end(),
        [&bc, id](const Node &other) -> bool { return other.uses(id); });
  };

  for (size_t i = bc.nodes.size(); --i;) {
    if (bc.nodes[i].op == sdfjit::bytecode::Op::Store_Result) {
      continue;
    }

    if (is_unused(i)) {
      bc.nodes[i].convert_to_nop();
    }
  }
}

} // namespace sdfjit::bytecode::passes
