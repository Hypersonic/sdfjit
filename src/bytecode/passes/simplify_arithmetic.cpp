#include "simplify_arithmetic.h"

#include "bytecode/bytecode.h"
#include "util/compare.h"

namespace sdfjit::bytecode::passes {

void simplify_arithmetic(Bytecode &bc) {
  /* Convert nodes like x * 1 to nops (and update all uses to uses of x)
   * Convert nodes like x * 0 to Assign_Float(0)
   * Convert nodes like x + 0 or x - 0 to nops, and update all uses to use node
   * x
   */

  auto operand_is_add_or_subtract_by_zero = [&bc](Node &node,
                                                  size_t arg_idx) -> bool {
    return (node.op == Op::Add || node.op == Op::Subtract) &&
           bc.nodes[node.arguments[arg_idx]].op == Op::Assign_Float &&
           util::floats_equal(bc.nodes[node.arguments[arg_idx]].value, 0.0f);
  };

  auto operand_is_multiply_by_one = [&bc](Node &node, size_t arg_idx) -> bool {
    return node.op == Op::Multiply &&
           bc.nodes[node.arguments[arg_idx]].op == Op::Assign_Float &&
           util::floats_equal(bc.nodes[node.arguments[arg_idx]].value, 1.0f);
  };

  auto operand_is_multiply_by_zero = [&bc](Node &node, size_t arg_idx) -> bool {
    return node.op == Op::Multiply &&
           bc.nodes[node.arguments[arg_idx]].op == Op::Assign_Float &&
           util::floats_equal(bc.nodes[node.arguments[arg_idx]].value, 0.0f);
  };

  for (size_t i = 0; i < bc.nodes.size(); i++) {
    auto &node = bc.nodes[i];

    // update adds by zero
    {
      auto is_lhs = operand_is_add_or_subtract_by_zero(node, 0);
      auto is_rhs = operand_is_add_or_subtract_by_zero(node, 1);
      if (is_lhs || is_rhs) {
        Node_Id value_node;
        if (is_lhs) {
          value_node = node.arguments[1];
        } else {
          value_node = node.arguments[0];
        }

        node.convert_to_nop();
        bc.replace_all_uses_with(i, value_node);
        continue;
      }
    }

    // update multiplies by one
    {
      auto is_lhs = operand_is_multiply_by_one(node, 0);
      auto is_rhs = operand_is_multiply_by_one(node, 1);
      if (is_lhs || is_rhs) {
        Node_Id value_node;
        if (is_lhs) {
          value_node = node.arguments[1];
        } else {
          value_node = node.arguments[0];
        }

        node.convert_to_nop();
        bc.replace_all_uses_with(i, value_node);
        continue;
      }
    }

    // update all multiplies by zero
    {
      auto is_lhs = operand_is_multiply_by_zero(node, 0);
      auto is_rhs = operand_is_multiply_by_zero(node, 1);
      if (is_lhs || is_rhs) {
        node.op = Op::Assign_Float;
        node.arguments.clear();
        node.value = 0.0f;
        continue;
      }
    }
  }
}

} // namespace sdfjit::bytecode::passes
