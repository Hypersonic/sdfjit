#pragma once

#include <iostream>
#include <variant>
#include <vector>

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
#define FOREACH_MACHINE_OP(macro) \
    macro(movaps, 2, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1)) \
    macro(vaddps, 3, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1, 2))
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

struct Register {
  // virtual = has not been assigned a concrete register
  // machine = has a concrete register assigned
  // memory  = register + offset assigned, as in [rdi + 0x1234]
  enum class Kind { Virtual, Machine, Memory };
  using Register_Type =
      std::variant<Virtual_Register, Machine_Register, Memory_Reference>;

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

  bool is_virtual() const { return kind == Kind::Virtual; }
  bool is_machine() const { return kind == Kind::Machine; }
  bool is_memory() const { return kind == Kind::Memory; }
  Virtual_Register virtual_reg() const {
    return std::get<Virtual_Register>(reg);
  }
  Machine_Register machine_reg() const {
    return std::get<Machine_Register>(reg);
  }
  Memory_Reference memory_ref() const {
    return std::get<Memory_Reference>(reg);
  }
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

  Instruction &add_instruction(const Instruction &insn) {
    instructions.push_back(insn);
    return instructions.back();
  }

  Register new_virtual_register() {
    return Register{Register::Kind::Virtual, next_virtual_register++};
  }

  Register movaps(const Register &src);
  Register vaddps(const Register &lhs, const Register &rhs);
};

std::ostream &operator<<(std::ostream &os, const Machine_Register reg);
std::ostream &operator<<(std::ostream &os, const Memory_Reference mem);
std::ostream &operator<<(std::ostream &os, const Op op);
std::ostream &operator<<(std::ostream &os, const Register reg);
std::ostream &operator<<(std::ostream &os, const Instruction &insn);
std::ostream &operator<<(std::ostream &os, const Machine_Code &mc);

} // namespace sdfjit::machinecode
