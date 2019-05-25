#include "constant_fold.h"

#include <algorithm>
#include <cmath>

#include "bytecode/bytecode.h"

namespace sdfjit::bytecode::passes {

void constant_fold(Bytecode &bc) {
  using sdfjit::bytecode::Op;

  auto can_optimize_arguments_node = [&bc](Node &node) {
    return std::all_of(node.arguments.begin(), node.arguments.end(),
                       [&bc](Node_Id id) -> bool {
                         return bc.nodes[id].op == Op::Assign_Float;
                       });
  };

  for (size_t i = 0; i < bc.nodes.size(); i++) {
    auto &node = bc.nodes[i];

    if (node.is_constant_expression(bc)) {
      switch (node.op) {
      case Op::Add: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = bc.nodes[node.arguments[0]].value +
                       bc.nodes[node.arguments[1]].value;
        }
        break;
      }

      case Op::Subtract: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = bc.nodes[node.arguments[0]].value -
                       bc.nodes[node.arguments[1]].value;
        }
        break;
      }

      case Op::Multiply: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = bc.nodes[node.arguments[0]].value *
                       bc.nodes[node.arguments[1]].value;
        }
        break;
      }

      case Op::Divide: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = bc.nodes[node.arguments[0]].value /
                       bc.nodes[node.arguments[1]].value;
        }
        break;
      }

      case Op::Sqrt: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = sqrtf(bc.nodes[node.arguments[0]].value);
        }
        break;
      }

      case Op::Abs: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = fabs(bc.nodes[node.arguments[0]].value);
        }
        break;
      }

      case Op::Negate: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = -bc.nodes[node.arguments[0]].value;
        }
        break;
      }

      case Op::Min: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = std::min(bc.nodes[node.arguments[0]].value,
                                bc.nodes[node.arguments[1]].value);
        }
        break;
      }

      case Op::Max: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = std::max(bc.nodes[node.arguments[0]].value,
                                bc.nodes[node.arguments[1]].value);
        }
        break;
      }

      case Op::Sin: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = sinf(bc.nodes[node.arguments[0]].value);
        }
        break;
      }

      case Op::Cos: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = cosf(bc.nodes[node.arguments[0]].value);
        }
        break;
      }

      case Op::Mod: {
        if (can_optimize_arguments_node(node)) {
          node.arguments.clear();
          node.op = Op::Assign_Float;
          node.value = fmodf(bc.nodes[node.arguments[0]].value,
                             bc.nodes[node.arguments[1]].value);
        }
        break;
      }

      default: {
        // we don't have an optimization for this node type (yet!)
        break;
      }
      }

      // we'd really like to convert nodes to nops, but we don't know if they're
      // completely unused yet, so we leave the Assign_Float around. A dead
      // store elimination pass should kill them later.
    }
  }
}

} // namespace sdfjit::bytecode::passes
