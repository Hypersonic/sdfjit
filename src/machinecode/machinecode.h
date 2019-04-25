#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include "bytecode/bytecode.h"
#include "util/bits.h"

namespace sdfjit::machinecode {

using Virtual_Register = size_t;

// clang-format off
// macro(register_name, register_number)
// TODO: find register numbers in docs, for now they're all 0
// XXX: we'll probably want to break this up by register family (normal, xmms, ymms, etc)
#define FOREACH_MACHINE_REGISTER(macro) \
    macro(rdi, 0) \
    macro(rsi, 0) \
    macro(rdx, 0) \
    macro(rcx, 0) \
    macro(r8, 0) \
    macro(r9, 0) \
    macro(xmm0, 0) \
    macro(xmm1, 0) \
    macro(xmm2, 0) \
    macro(xmm3, 0) \
    macro(xmm4, 0) \
    macro(xmm5, 0) \
    macro(xmm6, 0) \
    macro(xmm7, 0) \
    macro(xmm8, 0) \
    macro(xmm9, 0) \
    macro(xmm10, 0) \
    macro(xmm11, 0) \
    macro(xmm12, 0) \
    macro(xmm13, 0) \
    macro(xmm14, 0) \
    macro(xmm15, 0) \
    macro(ymm0, 0) \
    macro(ymm1, 0) \
    macro(ymm2, 0) \
    macro(ymm3, 0) \
    macro(ymm4, 0) \
    macro(ymm5, 0) \
    macro(ymm6, 0) \
    macro(ymm7, 0)

// macro(op_name, num_args, set_reg_idxs, used_reg_idxs)
// TODO: all the ops
// ops are generally named after their native code mneumonic
#define MC_INITIALIZER_LIST(...) { __VA_ARGS__ }

#define BINARY_MACHINE_OP_MACRO_WRAPPER(macro, name) \
    macro(name, 3, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1, 2)) \

#define UNARY_MACHINE_OP_MACRO_WRAPPER(macro, name) \
    macro(name, 2, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1)) \

#define FOREACH_BINARY_MACHINE_OP(macro) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vaddps) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vsubps) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vmulps) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vpslld) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vpsrld) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vmaxps) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vminps) \

#define FOREACH_UNARY_MACHINE_OP(macro) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, movd) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, movaps) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vbroadcastss) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vsqrtps) \


#define FOREACH_MACHINE_OP(macro) \
    FOREACH_UNARY_MACHINE_OP(macro) \
    FOREACH_BINARY_MACHINE_OP(macro)

// clang-format on

#define MACHINE_REGISTER_ENUM(register_name, register_number) register_name,
enum class Machine_Register { FOREACH_MACHINE_REGISTER(MACHINE_REGISTER_ENUM) };
#undef MACHINE_REGISTER_ENUM

#define MACHINE_OP_ENUM(op_name, ...) op_name,
enum class Op { FOREACH_MACHINE_OP(MACHINE_OP_ENUM) };
#undef MACHINE_OP_ENUM

struct Memory_Reference {
  // For the base register, same as the corresponding definitions in Register
  enum class Kind { Virtual, Machine };
  using Register_Type = std::variant<Virtual_Register, Machine_Register>;

  Kind kind;
  Register_Type base;
  size_t offset;

  bool is_virtual() const { return kind == Kind::Virtual; }
  bool is_machine() const { return kind == Kind::Machine; }
  Virtual_Register virtual_reg() const {
    return std::get<Virtual_Register>(base);
  }
  Machine_Register machine_reg() const {
    return std::get<Machine_Register>(base);
  }
};

using Immediate_Value = uint32_t;

struct Register {
  // virtual   = has not been assigned a concrete register
  // machine   = has a concrete register assigned
  // memory    = register + offset assigned, as in [rdi + 0x1234]
  // immediate = fixed 32-bit value
  enum class Kind { Virtual, Machine, Memory, Immediate };
  using Register_Type = std::variant<Virtual_Register, Machine_Register,
                                     Memory_Reference, Immediate_Value>;

  Kind kind;
  Register_Type reg;

  static Register Virtual(size_t reg) { return {Kind::Virtual, reg}; }
  static Register Machine(Machine_Register reg) { return {Kind::Machine, reg}; }
  static Register Memory(Virtual_Register base, size_t index) {
    return {Kind::Memory,
            Memory_Reference{Memory_Reference::Kind::Virtual, base, index}};
  }
  static Register Memory(Machine_Register base, size_t index) {
    return {Kind::Memory,
            Memory_Reference{Memory_Reference::Kind::Machine, base, index}};
  }
  static Register Imm(uint32_t imm) { return {Kind::Immediate, imm}; }
  static Register Imm(float imm) {
    return {Kind::Immediate, util::float_to_bits(imm)};
  }

  bool is_virtual() const { return kind == Kind::Virtual; }
  bool is_machine() const { return kind == Kind::Machine; }
  bool is_memory() const { return kind == Kind::Memory; }
  bool is_immediate() const { return kind == Kind::Immediate; }
  Virtual_Register virtual_reg() const {
    return std::get<Virtual_Register>(reg);
  }
  Machine_Register machine_reg() const {
    return std::get<Machine_Register>(reg);
  }
  Memory_Reference memory_ref() const {
    return std::get<Memory_Reference>(reg);
  }
  Immediate_Value imm() const { return std::get<Immediate_Value>(reg); }

}; // namespace sdfjit::machinecode

struct Instruction {
  Op op;
  std::vector<Register> registers; // some of these are in-regs, some out-regs.
                                   // Each Op has different requirements :cry:

  std::vector<Register> set_registers() const;
  std::vector<Register> used_registers() const;
};

struct Machine_Code {
  std::vector<Instruction> instructions{};
  Virtual_Register next_virtual_register{0};

  static Machine_Code from_bytecode(const sdfjit::bytecode::Bytecode &bc);

  Instruction &add_instruction(const Instruction &insn) {
    instructions.push_back(insn);
    return instructions.back();
  }

  Register new_virtual_register() {
    return Register{Register::Kind::Virtual, next_virtual_register++};
  }

#define UNARY_DECL(name, ...) Register name(const Register &src);
#define BINARY_DECL(name, ...)                                                 \
  Register name(const Register &lhs, const Register &rhs);

  FOREACH_UNARY_MACHINE_OP(UNARY_DECL);
  FOREACH_BINARY_MACHINE_OP(BINARY_DECL);

#undef BINARY_DECL
#undef UNARY_DECL
};

std::ostream &operator<<(std::ostream &os, const Machine_Register reg);
std::ostream &operator<<(std::ostream &os, const Memory_Reference mem);
std::ostream &operator<<(std::ostream &os, const Op op);
std::ostream &operator<<(std::ostream &os, const Register reg);
std::ostream &operator<<(std::ostream &os, const Instruction &insn);
std::ostream &operator<<(std::ostream &os, const Machine_Code &mc);

Register get_argument_register(size_t arg_index);

} // namespace sdfjit::machinecode
