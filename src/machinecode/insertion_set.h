#pragma once

#include "machinecode.h"

namespace sdfjit::machinecode {

enum class Insert_Side { Before, After };

struct Insert_At {
  Insert_Side side;
  size_t index;
  size_t insert_id;
  Instruction instruction;
};

struct Insertion_Set;
struct Inserter {
  Insert_Side side;
  Insertion_Set &set;

  void add_instruction(size_t index, const Instruction &instruction);

#define DECLARE_BINARY_OP(name, ...)                                           \
  void name(size_t index, const Register &result, const Register &lhs,         \
            const Register &rhs) {                                             \
    add_instruction(index, Instruction{Op::name, {result, lhs, rhs}});         \
  }
#define DECLARE_UNARY_OP(name, ...)                                            \
  void name(size_t index, const Register &result, const Register &src) {       \
    add_instruction(index, Instruction{Op::name, {result, src}});              \
  }
#define DECLARE_X86_BINARY_OP(name, ...)                                       \
  void name(size_t index, const Register &lhs, const Register &rhs) {          \
    add_instruction(index, Instruction{Op::name, {lhs, rhs}});                 \
  }
#define DECLARE_X86_UNARY_OP(name, ...)                                        \
  void name(size_t index, const Register &val) {                               \
    add_instruction(index, Instruction{Op::name, {val}});                      \
  }
#define DECLARE_X86_NULLARY_OP(name, ...)                                      \
  void name(size_t index) { add_instruction(index, Instruction{Op::name, {}}); }

  FOREACH_BINARY_MACHINE_OP(DECLARE_BINARY_OP);
  FOREACH_UNARY_MACHINE_OP(DECLARE_UNARY_OP);
  FOREACH_X86_BINARY_MACHINE_OP(DECLARE_X86_BINARY_OP);
  FOREACH_X86_UNARY_MACHINE_OP(DECLARE_X86_UNARY_OP);
  FOREACH_X86_NULLARY_MACHINE_OP(DECLARE_X86_NULLARY_OP);
};

struct Insertion_Set {
  Machine_Code &mc;
  std::vector<Insert_At> insertions{};
  size_t insert_count{0};

  Inserter before{Insert_Side::Before, *this};
  Inserter after{Insert_Side::After, *this};

  void commit();
};

} // namespace sdfjit::machinecode
