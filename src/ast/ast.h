#pragma once

#include <iostream>
#include <vector>

namespace sdfjit::ast {

// clang-format off
// macro(op_type)
// XXX: I've limited this to a simple set of ast ops so far, there are a lot more but I want to be able to experiment pretty easily
#define FOREACH_AST_OP(macro) \
    /* Primitive types */ \
    macro(Sphere) \
    macro(Box) \
    macro(Float32) \
    macro(Pos3) \
    macro(Noop) \
    /* Composition operators */ \
    macro(Add) \
    macro(Subtract) \
    macro(Intersect) \
    /* Movement operators */ \
    macro(Rotate) \
    macro(Translate) \
    macro(Scale)

// clang-format on

#define DEFINE_ENUM_MEMBER_FOR_OP(op_type) op_type,
enum class Op { FOREACH_AST_OP(DEFINE_ENUM_MEMBER_FOR_OP) };
#undef DEFINE_ENUM_MEMBER_FOR_OP
std::ostream &operator<<(std::ostream &os, Op op);

// positive = indexes in the ast's nodes list.
// negative = input parameters
using Node_Id = int32_t;

constexpr Node_Id IN_X = -1;
constexpr Node_Id IN_Y = -2;
constexpr Node_Id IN_Z = -3;
constexpr Node_Id IN_CONSTANTS = -4;
constexpr Node_Id OUT_PTR = -5;

struct Ast;

struct Node {
  Op op;
  // XXX: do we want to stuff these together into a variant?
  std::vector<Node_Id> children; // for non-float32 nodes
  float value{0.};               // for float32 nodes

  bool is_same_as(Ast &ast, Node &other);
};

struct Ast {
  std::vector<Node> nodes{};

  Node_Id add_node(const Node &node) {
    nodes.push_back(node);
    return nodes.size() - 1;
  }

  Node_Id root_node_id() const { return nodes.size() - 1; }

  void dump(std::ostream &os);
  void dump_sexpr(std::ostream &os, size_t indent = 0);

  /* Primitives */
  Node_Id sphere(Node_Id position, float radius);
  Node_Id sphere(Node_Id position, Node_Id radius);
  Node_Id box(Node_Id position, float wx, float wy, float wz);
  Node_Id box(Node_Id position, Node_Id wx, Node_Id wy, Node_Id wz);
  Node_Id float32(float value);
  Node_Id pos3(float x, float y, float z);
  Node_Id pos3(Node_Id x, Node_Id y, Node_Id z);
  /* Composition Operators */
  Node_Id add(Node_Id lhs, Node_Id rhs);
  Node_Id subtract(Node_Id lhs, Node_Id rhs);
  Node_Id intersect(Node_Id lhs, Node_Id rhs);
  /* Movement operators */
  // remember that these operate on a position and return a modified position,
  // not on an object. Pass the modified position into that object as the first
  // parameter to operate on that object.
  Node_Id rotate(Node_Id position, float rx, float ry, float rz);
  Node_Id rotate(Node_Id position, Node_Id rotation);
  Node_Id translate(Node_Id position, float rx, float ry, float rz);
  Node_Id translate(Node_Id position, Node_Id translation);
  Node_Id scale(Node_Id position, float rx, float ry, float rz);
  Node_Id scale(Node_Id position, Node_Id scale);

  // convert a node to a noop, and break dependence on it's children
  void kill(Node_Id id) {
    auto &node = nodes.at(id);
    node.op = Op::Noop;
    node.children.clear();
  }

  // replace all uses of id `from` with id `to`
  void replace_all_uses_with(Node_Id from, Node_Id to);
};

} // namespace sdfjit::ast
