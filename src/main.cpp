#include <iostream>

#include "ast/ast.h"
#include "ast/opt.h"

#include "bytecode/bytecode.h"

int main() {
  sdfjit::ast::Ast ast{};
  auto pos = ast.pos3(sdfjit::ast::IN_X, sdfjit::ast::IN_Y,
                      sdfjit::ast::IN_Z); // x, y, z input parameters.

  ast.add(ast.box(pos, 10.0f, 20.0f, 30.0f),
          ast.sphere(ast.translate(pos, 30.0f, 30.0f, 30.0f), 6.0f));

  sdfjit::ast::opt::optimize(ast);

  std::cout << "AST (sexpr):" << std::endl;
  ast.dump_sexpr(std::cout);
  std::cout << "=====================" << std::endl;
  std::cout << "AST (linearized):" << std::endl;
  ast.dump(std::cout);
  std::cout << "=====================" << std::endl;

  auto bc = sdfjit::bytecode::Bytecode::from_ast(ast);

  std::cout << "Bytecode:" << std::endl;
  bc.dump(std::cout);
}
