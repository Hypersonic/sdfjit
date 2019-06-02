#include <cmath>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "ast/ast.h"
#include "ast/opt.h"
#include "bytecode/bytecode.h"
#include "bytecode/opt.h"
#include "machinecode/assembler.h"
#include "machinecode/executor.h"
#include "machinecode/machinecode.h"
#include "machinecode/opt.h"
#include "profiling/frame_counter.h"
#include "profiling/perf_map_writer.h"
#include "raytracer/raytracer.h"
#include "util/hexdump.h"

sdfjit::ast::Ast ast_at(size_t t) {
  sdfjit::ast::Ast ast{};
  auto pos = ast.pos3(sdfjit::ast::IN_X, sdfjit::ast::IN_Y,
                      sdfjit::ast::IN_Z); // x, y, z input parameters.

  pos = ast.translate(pos, 0.0f, 0.0f, -200.0f);

  auto merged =
      ast.add(ast.box(ast.rotate(pos, 0.0f, t / 20.0f, 0.0f), 10.0f, 20.0f,
                      30.0f, 2.0f),
              ast.sphere(ast.translate(pos, 30.0f, 30.0f, 30.0f), 6.0f, 1.0f));

  merged = ast.add(
      merged, ast.box(ast.rotate(ast.translate(pos, -60.0f, -60.0f, -60.0f),
                                 t / 10.0f, t / 30.0f, 0.0f),
                      20.0f, 20.0f, 20.0f, 3.0f));
  return ast;
}

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

  sdfjit::bytecode::optimize(bc);
  std::cout << "Bytecode (optimized):" << std::endl;
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

  std::cout << "Machine Code (late optimizations applied):" << std::endl;
  sdfjit::machinecode::optimize(mc);
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

void render_animation() {
  size_t width = 320;
  size_t height = 240;
  uint32_t *screen = (uint32_t *)malloc(width * height * sizeof(uint32_t));

  auto rgb = [](uint32_t val) -> std::tuple<int, int, int> {
    return {int((val & 0xff0000) >> 16), int((val & 0xff00) >> 8),
            int(val & 0xff)};
  };

  mkdir("frames", 0777);
  mkdir("jits", 0777);

  Frame_Counter fps{};
  for (size_t t = 0; t < 300; t++) {
    auto ast = ast_at(t);
    sdfjit::ast::opt::optimize(ast);

    auto rt = sdfjit::raytracer::Raytracer::from_ast(ast);
    sdfjit::profiling::add_perf_map_region(rt.exec,
                                           "frame" + std::to_string(t));
    rt.trace_image(0, 0, 0, 0, 0, 0, width, height, screen);

    {
      std::fstream out("frames/out" + std::to_string(t) + ".ppm",
                       std::fstream::out);
      out << "P3\n" << width << " " << height << "\n255\n";
      for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
          size_t offset = y * width + x;
          auto [r, g, b] = rgb(screen[offset]);
          out << r << " " << g << " " << b << " ";
        }
        out << "\n";
      }
      out.close();
    }

    {
      std::fstream out("jits/jit" + std::to_string(t) + ".txt",
                       std::fstream::out);
      out << rt.exec.mc;
      out.close();
    }

    std::cout << "done with frame " << t << std::endl;
    fps.tick_frame();
  }
  fps.stop();
  std::cout << "done rendering at " << fps.fps() << " fps" << std::endl;
}

int main() {
  auto ast = ast_at(0);
  sdfjit::ast::opt::optimize(ast);

  dump_all_parts(ast);

  auto rt = sdfjit::raytracer::Raytracer::from_ast(ast);

  auto width = 320;
  auto height = 240;
  uint32_t *screen = (uint32_t *)malloc(width * height * sizeof(uint32_t));
  rt.trace_image(0, 0, 0, 0, 0, 0, width, height, screen);

  render_animation();
}
