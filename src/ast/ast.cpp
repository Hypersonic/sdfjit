#include "ast.h"

namespace sdfjit::ast {

bool Node::is_same_as(Ast &ast, Node &other) {
  (void)ast;

  if (op != other.op) {
    return false;
  }

  if (op == Op::Noop) {
    // noops are always 'unique'
    return false;
  }

  if (op == Op::Float32) {
    return std::abs(value - other.value) < .00001;
  }

  if (children.size() != other.children.size()) {
    return false;
  }
  for (size_t i = 0; i < children.size(); i++) {
    if (children[i] != other.children[i]) {
      return false;
    }
  }
  return true;
}

std::ostream &operator<<(std::ostream &os, Op op) {
#define OP_OUTPUT(op_type)                                                     \
  case Op::op_type:                                                            \
    return os << #op_type;

  switch (op) { FOREACH_AST_OP(OP_OUTPUT); }

#undef OP_OUTPUT
  abort(); // unreachable
}

void Ast::dump(std::ostream &os) {
  for (size_t i = 0; i < nodes.size(); i++) {
    auto &node = nodes[i];

    os << '@' << i << ": ";
    os << node.op << '(';
    if (node.op == Op::Float32) {
      os << node.value;
    } else {
      // this will put an extra , on the last one but whatever
      for (const auto child_id : node.children) {
        os << '@' << child_id << ',';
      }
    }

    os << ')' << std::endl;
  }
}

void dump_node_sexpr(const Ast &ast, Node_Id id, std::ostream &os,
                     size_t indent) {
  auto emit_indent = [&os](size_t indent_) {
    for (size_t i = 0; i < indent_; i++) {
      os << "  ";
    }
  };

  emit_indent(indent);
  os << '(' << ast.nodes[id].op << std::endl;
  if (ast.nodes[id].op == Op::Float32) {
    emit_indent(indent + 1);
    os << ast.nodes[id].value << std::endl;
  } else {
    for (auto child_id : ast.nodes[id].children) {
      if (child_id < 0) {
        emit_indent(indent + 1);
        std::cout << "(arg " << std::abs(child_id) - 1 << ')' << std::endl;
      } else {
        dump_node_sexpr(ast, child_id, os, indent + 1);
      }
    }
  }

  emit_indent(indent);
  os << ')' << std::endl;
}

void Ast::dump_sexpr(std::ostream &os, size_t indent) {
  dump_node_sexpr(*this, root_node_id(), os, indent);
}

/* Primitives */

Node_Id Ast::sphere(Node_Id position, float radius) {
  return sphere(position, float32(radius));
}

Node_Id Ast::sphere(Node_Id position, Node_Id radius) {
  return add_node(Node{Op::Sphere, {position, radius}});
}

Node_Id Ast::box(Node_Id position, float wx, float wy, float wz) {
  return box(position, float32(wx), float32(wy), float32(wz));
}

Node_Id Ast::box(Node_Id position, Node_Id wx, Node_Id wy, Node_Id wz) {
  return add_node(Node{Op::Box, {position, wx, wy, wz}});
}

Node_Id Ast::float32(float value) { return add_node({Op::Float32, {}, value}); }

Node_Id Ast::pos3(float x, float y, float z) {
  return pos3(float32(x), float32(y), float32(z));
}

Node_Id Ast::pos3(Node_Id x, Node_Id y, Node_Id z) {
  return add_node(Node{Op::Pos3, {x, y, z}});
}

/* Composition Operators */

Node_Id Ast::add(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Add, {lhs, rhs}});
}

Node_Id Ast::subtract(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Subtract, {lhs, rhs}});
}

Node_Id Ast::intersect(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Intersect, {lhs, rhs}});
}

/* Movement operators */

Node_Id Ast::rotate(Node_Id position, float rx, float ry, float rz) {
  return rotate(position, pos3(rx, ry, rz));
}

Node_Id Ast::rotate(Node_Id position, Node_Id rotation) {
  return add_node(Node{Op::Rotate, {position, rotation}});
}

Node_Id Ast::translate(Node_Id position, float rx, float ry, float rz) {
  return translate(position, pos3(rx, ry, rz));
}

Node_Id Ast::translate(Node_Id position, Node_Id translation) {
  return add_node(Node{Op::Translate, {position, translation}});
}

Node_Id Ast::scale(Node_Id position, float rx, float ry, float rz) {
  return scale(position, pos3(rx, ry, rz));
}

Node_Id Ast::scale(Node_Id position, Node_Id scale) {
  return add_node(Node{Op::Scale, {position, scale}});
}

void Ast::replace_all_uses_with(Node_Id from, Node_Id to) {
  for (auto &node : nodes) {
    if (node.op == Op::Float32)
      continue;

    for (auto &id : node.children) {
      if (id == from) {
        id = to;
      }
    }
  }
}

} // namespace sdfjit::ast
