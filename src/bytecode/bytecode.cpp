#include "bytecode.h"

#include <cmath>
#include <unordered_map>

namespace sdfjit::bytecode {

std::ostream &operator<<(std::ostream &os, Op op) {
#define STRING_FROM_OP(op_type)                                                \
  case Op::op_type:                                                            \
    return os << #op_type;

  switch (op) { FOREACH_BC_OP(STRING_FROM_OP); }

#undef STRING_FROM_OP

  abort(); // unreachable
}

void Bytecode::dump(std::ostream &os) {
  for (size_t i = 0; i < nodes.size(); i++) {
    auto &node = nodes[i];
    os << '@' << i << ": ";
    os << node.op << '(';
    if (node.op == Op::Assign_Float) {
      os << node.value;
    } else if (node.op == Op::Load_Arg) {
      os << node.arg_index;
    } else {
      for (const auto arg_id : node.arguments) {
        os << '@' << arg_id << ", ";
      }
    }
    os << ')' << std::endl;
  }
}

Bytecode Bytecode::from_ast(sdfjit::ast::Ast &ast) {
  Bytecode bc{};

  // maps the id of an ast operation to the bytecode node containing it.
  std::unordered_map<sdfjit::ast::Node_Id, Node_Id> ast_to_bc{};
  // map an ast id to a list of "results" generated by that node's bytecode
  // For example, a pos3 generates a result list of size 3 for x, y, and z,
  // respectively various transforms do similar.
  std::unordered_map<sdfjit::ast::Node_Id, std::vector<Node_Id>> ast_results{};

  // first, add args to the bytecode for the argument indices:
  auto arg_x = bc.load_arg(0);
  auto arg_y = bc.load_arg(1);
  auto arg_z = bc.load_arg(2);
  auto arg_constants = bc.load_arg(3);

  ast_to_bc[sdfjit::ast::IN_X] = arg_x;
  ast_results[sdfjit::ast::IN_X] = {arg_x};
  ast_to_bc[sdfjit::ast::IN_Y] = arg_y;
  ast_results[sdfjit::ast::IN_Y] = {arg_y};
  ast_to_bc[sdfjit::ast::IN_Z] = arg_z;
  ast_results[sdfjit::ast::IN_Z] = {arg_z};
  ast_to_bc[sdfjit::ast::IN_CONSTANTS] = arg_constants;
  ast_results[sdfjit::ast::IN_CONSTANTS] = {arg_constants};

  for (size_t i = 0; i < ast.nodes.size(); i++) {
    const auto &node = ast.nodes[i];
    /*
    std::cout << "converting ast index to bytecode " << i << std::endl;
    std::cout << "Ast results:" << std::endl;
    for (const auto &[k, v] : ast_results) {
      std::cout << k << ": " << v.size() << std::endl;
    }
    */

    switch (node.op) {
    case sdfjit::ast::Op::Sphere: {
      /*
      float sdSphere( vec3 p, float s )
      {
        return length(p)-s;
      }
      */

      // get the x, y, and z out of the passed in position
      auto &position_results = ast_results.at(node.children.at(0));
      auto position_x = position_results.at(0);
      auto position_y = position_results.at(1);
      auto position_z = position_results.at(2);

      // length(p) = sqrt(x*x + y*y + z*z)
      auto x_sq = bc.multiply(position_x, position_x);
      auto y_sq = bc.multiply(position_y, position_y);
      auto z_sq = bc.multiply(position_z, position_z);
      auto length = bc.sqrt(bc.add(x_sq, bc.add(y_sq, z_sq)));

      auto radius = ast_to_bc.at(node.children.at(1));

      auto result = bc.subtract(length, radius);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Box: {
      /*
      float sdBox( vec3 p, vec3 b )
      {
        vec3 d = abs(p) - b;
        return length(max(d,0.0)) + min(max(d.x,max(d.y,d.z)),0.0);
      }
      */

      // get the x, y, and z out of the passed in position
      auto &position_results = ast_results.at(node.children.at(0));
      auto position_x = position_results.at(0);
      auto position_y = position_results.at(1);
      auto position_z = position_results.at(2);

      auto box_wx = ast_to_bc.at(node.children.at(1));
      auto box_wy = ast_to_bc.at(node.children.at(2));
      auto box_wz = ast_to_bc.at(node.children.at(3));

      // d = abs(p) - b
      auto d_x = bc.subtract(bc.abs(position_x), box_wx);
      auto d_y = bc.subtract(bc.abs(position_y), box_wy);
      auto d_z = bc.subtract(bc.abs(position_z), box_wz);

      // length(max(d, 0.0))
      auto zero = bc.assign_float(0.0f);
      auto d_x_max = bc.max(d_x, zero);
      auto d_y_max = bc.max(d_y, zero);
      auto d_z_max = bc.max(d_z, zero);
      auto d_x_sq = bc.multiply(d_x_max, d_x_max);
      auto d_y_sq = bc.multiply(d_y_max, d_y_max);
      auto d_z_sq = bc.multiply(d_z_max, d_z_max);
      auto length = bc.sqrt(bc.add(d_x_sq, bc.add(d_y_sq, d_z_sq)));

      // min(max(d.x,max(d.y,d.z), 0.0)
      auto minmax = bc.min(bc.max(d_x, bc.max(d_y, d_z)), zero);

      auto result = bc.add(length, minmax);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Float32: {
      auto result = bc.assign_float(node.value);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Pos3: {
      // we don't actually emit anything, but we do setup ast_results
      // appropriately.
      auto x = ast_to_bc.at(node.children.at(0));
      auto y = ast_to_bc.at(node.children.at(1));
      auto z = ast_to_bc.at(node.children.at(2));
      ast_results[i] = {x, y, z};
      break;
    }

    case sdfjit::ast::Op::Noop: {
      break;
    }

    case sdfjit::ast::Op::Add: {
      // remember that Add is actually Union
      /* float opUnion( float d1, float d2 ) {  min(d1,d2); } */
      auto lhs = ast_to_bc.at(node.children[0]);
      auto rhs = ast_to_bc.at(node.children[1]);
      auto result = bc.min(lhs, rhs);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Subtract: {
      /* float opSubtraction( float d1, float d2 ) { return max(-d1,d2); } */
      auto lhs = ast_to_bc.at(node.children[0]);
      auto rhs = ast_to_bc.at(node.children[1]);
      auto result = bc.max(bc.negate(lhs), rhs);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Intersect: {
      /* float opIntersection( float d1, float d2 ) { return max(d1,d2); } */
      auto lhs = ast_to_bc.at(node.children[0]);
      auto rhs = ast_to_bc.at(node.children[1]);
      auto result = bc.max(lhs, rhs);
      ast_to_bc[i] = result;
      ast_results[i] = {result};
      break;
    }

    case sdfjit::ast::Op::Rotate: {
      auto position = ast_results.at(node.children.at(0));
      auto x = position.at(0);
      auto y = position.at(1);
      auto z = position.at(2);

      auto rotate = ast_results.at(node.children.at(1));
      auto rx = rotate.at(0);
      auto ry = rotate.at(1);
      auto rz = rotate.at(2);

      /* Quick reminder on rotation matrices:
       *
       *      [ 1    0     0   ]
       * Rx = [ 0    cos  -sin ]
       *      [ 0    sin   cos ]
       *
       *      [ cos  0     sin ]
       * Ry = [ 0    1     0   ]
       *      [-sin  0     cos ]
       *
       *      [ cos -sin   0   ]
       * Rz = [ sin  cos   0   ]
       *      [ 0    0     1   ]
       *
       */

      auto sinrx = bc.sin(rx);
      auto cosrx = bc.cos(rx);
      auto sinry = bc.sin(ry);
      auto cosry = bc.cos(ry);
      auto sinrz = bc.sin(rz);
      auto cosrz = bc.cos(rz);

      // TODO: instead of doing a separate multiplication for each dimension,
      //       we should be doing the combined multiplication of all 3 matrices

      // rotate about x:
      // x' = x
      // y' = y * cos(t) - z * sin(t)
      // z' = y * sin(t) + z * cos(t)
      {
        auto x_prime = x;
        auto y_prime =
            bc.subtract(bc.multiply(y, cosrx), bc.multiply(z, sinrx));
        auto z_prime = bc.add(bc.multiply(y, sinrx), bc.multiply(z, cosrx));

        x = x_prime;
        y = y_prime;
        z = z_prime;
      }

      // rotate about y:
      // x' = x * cos(t) + z * sin(t)
      // y' = y
      // z' = x * -sin(t) + z * cos(t)
      {
        auto x_prime = bc.add(bc.multiply(x, cosry), bc.multiply(z, sinry));
        auto y_prime = y;
        auto z_prime =
            bc.add(bc.multiply(x, bc.negate(sinry)), bc.multiply(z, cosry));

        x = x_prime;
        y = y_prime;
        z = z_prime;
      }

      // rotate about z:
      // x' = x * cos(t) - y * sin(t)
      // y' = x * sin(t) + y * cos(t)
      // z' = z
      {
        auto x_prime =
            bc.subtract(bc.multiply(x, cosrz), bc.multiply(y, sinrz));
        auto y_prime = bc.add(bc.multiply(x, sinrz), bc.multiply(y, cosrz));
        auto z_prime = z;

        x = x_prime;
        y = y_prime;
        z = z_prime;
      }

      ast_results[i] = {x, y, z};
      break;
    }

    case sdfjit::ast::Op::Translate: {
      /* x += dx;
       * y += dy;
       * z += dz;
       */
      auto position = ast_results.at(node.children.at(0));
      auto x = position.at(0);
      auto y = position.at(1);
      auto z = position.at(2);

      auto delta = ast_results.at(node.children.at(1));
      auto dx = delta.at(0);
      auto dy = delta.at(1);
      auto dz = delta.at(2);

      auto new_x = bc.subtract(x, dx);
      auto new_y = bc.subtract(y, dy);
      auto new_z = bc.subtract(z, dz);

      ast_results[i] = {new_x, new_y, new_z};
      break;
    }

    case sdfjit::ast::Op::Scale: {
      abort(); // TODO: implement
      break;
    }
    }
  }

  bc.store_result(ast_to_bc.at(ast.nodes.size() - 1));

  return bc;
}

Node_Id Bytecode::load_arg(size_t arg_idx) {
  return add_node(Node{Op::Load_Arg, {}, 0.0f, arg_idx});
}

Node_Id Bytecode::store_result(Node_Id value) {
  return add_node(Node{Op::Store_Result, {value}});
}

Node_Id Bytecode::assign(Node_Id rhs) {
  return add_node(Node{Op::Assign, {rhs}});
}

Node_Id Bytecode::assign_float(float rhs) {
  return add_node(Node{Op::Assign_Float, {}, rhs});
}

Node_Id Bytecode::add(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Add, {lhs, rhs}});
}

Node_Id Bytecode::subtract(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Subtract, {lhs, rhs}});
}

Node_Id Bytecode::multiply(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Multiply, {lhs, rhs}});
}

Node_Id Bytecode::divide(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Divide, {lhs, rhs}});
}

Node_Id Bytecode::sqrt(Node_Id value) {
  return add_node(Node{Op::Sqrt, {value}});
}

Node_Id Bytecode::rsqrt(Node_Id value) {
  return add_node(Node{Op::Rsqrt, {value}});
}

Node_Id Bytecode::abs(Node_Id value) {
  return add_node(Node{Op::Abs, {value}});
}

Node_Id Bytecode::negate(Node_Id value) {
  return add_node(Node{Op::Negate, {value}});
}

Node_Id Bytecode::min(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Min, {lhs, rhs}});
}

Node_Id Bytecode::max(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Max, {lhs, rhs}});
}

Node_Id Bytecode::sin(Node_Id val) { return add_node(Node{Op::Sin, {val}}); }

Node_Id Bytecode::cos(Node_Id val) { return add_node(Node{Op::Cos, {val}}); }

Node_Id Bytecode::mod(Node_Id lhs, Node_Id rhs) {
  return add_node(Node{Op::Mod, {lhs, rhs}});
}

} // namespace sdfjit::bytecode
