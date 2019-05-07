#include "combine_identical_nodes.h"

#include "util/macros.h"

namespace sdfjit::ast::opt {

void combine_identical_nodes(Ast &ast) {

  for (size_t i = 0; i < ast.nodes.size(); i++) {
    auto &node = ast.nodes[i];

    for (size_t j = i + 1; j < ast.nodes.size(); j++) {
      auto &other = ast.nodes[j];
      if (UNLIKELY(node.is_same_as(ast, other))) {
        ast.replace_all_uses_with(j, i);
        ast.kill(j);
      }
    }
  }
}

} // namespace sdfjit::ast::opt
