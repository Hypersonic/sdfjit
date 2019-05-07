#pragma once

#include <iostream>
#include <vector>

#include "ast/ast.h"

namespace sdfjit::bytecode {

// clang-format off
// macro(op_type)
// XXX: I'm basically adding ops here as I need them
#define FOREACH_BC_OP(macro) \
    macro(Load_Arg) \
    macro(Store_Result) \
    macro(Assign) \
    macro(Assign_Float) \
    macro(Add) \
    macro(Subtract) \
    macro(Multiply) \
    macro(Divide) \
    macro(Sqrt) \
    macro(Rsqrt) \
    macro(Abs) \
    macro(Negate) \
    macro(Min) \
    macro(Max)
// clang-format on

#define DEFINE_ENUM_MEMBER_FOR_OP(op_type) op_type,
enum class Op { FOREACH_BC_OP(DEFINE_ENUM_MEMBER_FOR_OP) };
#undef DEFINE_ENUM_MEMBER_FOR_OP
std::ostream &operator<<(std::ostream &os, Op op);

// positive = indexes in the bytecode's nodes list.
// negative = input parameters
using Node_Id = int32_t;

struct Node {
  Op op;
  std::vector<Node_Id> arguments; // if has_arguments()
  float value{0.0};               // for Assign_Float
  size_t arg_index{0};            // for Load_Arg

  bool has_arguments() const {
    return op != Op::Assign_Float && op != Op::Load_Arg;
  }
};

struct Bytecode {
  std::vector<Node> nodes{};

  Node_Id add_node(Node node) {
    nodes.push_back(node);
    return nodes.size() - 1;
  }

  void dump(std::ostream &os);

  static Bytecode from_ast(sdfjit::ast::Ast &ast);

  Node_Id load_arg(size_t arg_idx);
  Node_Id store_result(Node_Id result);
  Node_Id assign(Node_Id rhs);
  Node_Id assign_float(float rhs);
  Node_Id add(Node_Id lhs, Node_Id rhs);
  Node_Id subtract(Node_Id lhs, Node_Id rhs);
  Node_Id multiply(Node_Id lhs, Node_Id rhs);
  Node_Id divide(Node_Id lhs, Node_Id rhs);
  Node_Id sqrt(Node_Id value);
  Node_Id rsqrt(Node_Id value);
  Node_Id abs(Node_Id value);
  Node_Id negate(Node_Id value);
  Node_Id min(Node_Id lhs, Node_Id rhs);
  Node_Id max(Node_Id lhs, Node_Id rhs);
};

} // namespace sdfjit::bytecode
