#include "cse.h"

namespace sdfjit::bytecode::passes {

void common_subexpression_elimination(Bytecode &bc) {
  for (size_t i = 0; i < bc.nodes.size(); i++) {
    for (size_t j = i + 1; j < bc.nodes.size(); j++) {
      if (bc.nodes[i] != bc.nodes[j]) {
        continue;
      }

      bc.replace_all_uses_with(j, i);
      bc.nodes[j].convert_to_nop();
    }
  }
  return;
}

} // namespace sdfjit::bytecode::passes
