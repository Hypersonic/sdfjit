#include <iostream>

#include "ast/ast.h"
#include "ast/opt.h"
#include "bytecode/bytecode.h"
#include "machinecode/assembler.h"
#include "machinecode/executor.h"
#include "machinecode/machinecode.h"
#include "raytracer/raytracer.h"
#include "util/hexdump.h"

void dump_all_parts(sdfjit::ast::Ast &ast) {
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
  mc.add_prologue_and_epilogue(); // XXX: maybe we should add the prologue
                                  // before emitting all the instructions...

  std::cout
      << "Machine Code (registers allocated, prologue and epilogue added):"
      << std::endl;
  std::cout << mc;
  std::cout << "Constant Pool:" << std::endl;
  std::cout << mc.constants;
  std::cout << "=====================" << std::endl;

  sdfjit::machinecode::Assembler assembler{mc};
  assembler.assemble();
  std::cout << "Asssembled instructions (" << assembler.buffer.size()
            << " bytes):" << std::endl;
  std::cout << assembler;
  std::cout << "Hexdump of that:" << std::endl;
  sdfjit::util::hexdump(std::cout, assembler.buffer);
  std::cout << "=====================" << std::endl;

  sdfjit::machinecode::Executor executor{mc};
  executor.create();
}

int main() {
  sdfjit::ast::Ast ast{};
  auto pos = ast.pos3(sdfjit::ast::IN_X, sdfjit::ast::IN_Y,
                      sdfjit::ast::IN_Z); // x, y, z input parameters.

  pos = ast.translate(pos, 0.0f, 0.0f, -200.0f);

  auto merged =
      ast.add(ast.box(pos, 10.0f, 20.0f, 30.0f),
              ast.sphere(ast.translate(pos, 30.0f, 30.0f, 30.0f), 6.0f));

  merged = ast.add(merged, ast.box(ast.translate(pos, -60.0f, -60.0f, -60.0f),
                                   20.0f, 20.0f, 20.0f));

  sdfjit::ast::opt::optimize(ast);

  dump_all_parts(ast);

  auto rt = sdfjit::raytracer::Raytracer::from_ast(ast);

  std::cout << "Exec page @ " << std::hex << rt.exec.code << std::dec
            << std::endl;

  for (float i = -50; i < 50; i++) {
    uint32_t *screen = (uint32_t *)malloc(320 * 240 * sizeof(uint32_t));
    rt.trace_image(0, 0, i, 0, 0, 0, 320, 240, screen);
  }
}
