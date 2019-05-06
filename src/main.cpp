#include <iostream>

#include "ast/ast.h"
#include "ast/opt.h"

#include "bytecode/bytecode.h"

#include "machinecode/machinecode.h"

#include "machinecode/assembler.h"

int main() {
  sdfjit::ast::Ast ast{};
  auto pos = ast.pos3(sdfjit::ast::IN_X, sdfjit::ast::IN_Y,
                      sdfjit::ast::IN_Z); // x, y, z input parameters.

  auto merged =
      ast.add(ast.box(pos, 10.0f, 20.0f, 30.0f),
              ast.sphere(ast.translate(pos, 30.0f, 30.0f, 30.0f), 6.0f));

  merged = ast.add(merged, ast.box(ast.translate(pos, -60.0f, -60.0f, -60.0f),
                                   20.0f, 20.0f, 20.0f));

  sdfjit::ast::opt::optimize(ast);

  std::cout << "AST (sexpr):" << std::endl;
  ast.dump_sexpr(std::cout);
  std::cout << "=====================" << std::endl;
  std::cout << "AST (linearized + CSE pass):" << std::endl;
  ast.dump(std::cout);
  std::cout << "=====================" << std::endl;

  std::cout << "Bytecode:" << std::endl;
  auto bc = sdfjit::bytecode::Bytecode::from_ast(ast);
  bc.dump(std::cout);
  std::cout << "=====================" << std::endl;

  std::cout << "Machine Code (imms inline, before register alloc):"
            << std::endl;
  auto mc = sdfjit::machinecode::Machine_Code::from_bytecode(bc);
  std::cout << mc;
  std::cout << "=====================" << std::endl;

  std::cout << "Machine Code (imms resolved, before register alloc):"
            << std::endl;
  mc.resolve_immediates();
  std::cout << mc;
  std::cout << "Constant Pool:" << std::endl;
  std::cout << mc.constants;
  std::cout << "=====================" << std::endl;

  mc.allocate_registers();

  std::cout << "Machine Code (registers allocated):" << std::endl;
  std::cout << mc;
  std::cout << "Constant Pool:" << std::endl;
  std::cout << mc.constants;
  std::cout << "=====================" << std::endl;

  std::cout << "Asssembled instructions:" << std::endl;
  sdfjit::machinecode::Assembler assembler{mc};
  for (const auto &instruction : mc.instructions) {
    std::cout << instruction << std::endl;
    assembler.buffer.clear();
    assembler.assemble_instruction(instruction);
    std::cout << assembler;
  }
}
