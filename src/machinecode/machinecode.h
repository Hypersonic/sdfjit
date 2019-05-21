#pragma once

#include <iostream>
#include <variant>
#include <vector>

#include "bytecode/bytecode.h"
#include "constantpool.h"
#include "stack.h"
#include "util/bits.h"

namespace sdfjit::machinecode {

using Virtual_Register = size_t;

// clang-format off
// macro(register_name, register_number)
// TODO: find register numbers in docs, for now they're all 0
// XXX: we'll probably want to break this up by register family (normal, xmms, ymms, etc)
#define FOREACH_MACHINE_REGISTER(macro) \
    macro(rax, 0) \
    macro(rcx, 1) \
    macro(rdx, 2) \
    macro(rsp, 4) \
    macro(rbp, 5) \
    macro(rsi, 6) \
    macro(rdi, 7) \
    macro(r8, 8) \
    macro(r9, 9) \
    macro(xmm0, 0) \
    macro(xmm1, 1) \
    macro(xmm2, 2) \
    macro(xmm3, 3) \
    macro(xmm4, 4) \
    macro(xmm5, 5) \
    macro(xmm6, 6) \
    macro(xmm7, 7) \
    macro(xmm8, 8) \
    macro(xmm9, 9) \
    macro(xmm10, 10) \
    macro(xmm11, 11) \
    macro(xmm12, 12) \
    macro(xmm13, 13) \
    macro(xmm14, 14) \
    macro(xmm15, 15) \
    macro(ymm0, 0) \
    macro(ymm1, 1) \
    macro(ymm2, 2) \
    macro(ymm3, 3) \
    macro(ymm4, 4) \
    macro(ymm5, 5) \
    macro(ymm6, 6) \
    macro(ymm7, 7)

// macro(op_name, num_args, set_reg_idxs, used_reg_idxs, takes_imm)
// TODO: all the ops
// ops are generally named after their native code mneumonic
#define MC_INITIALIZER_LIST(...) { __VA_ARGS__ }

#define BINARY_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 3, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1, 2), takes_imm) \

#define X86_BINARY_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 2, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(0, 1), takes_imm)

#define UNARY_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 2, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(1), takes_imm) \

#define X86_UNARY_IN_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 1, MC_INITIALIZER_LIST(), MC_INITIALIZER_LIST(0), takes_imm)

#define X86_UNARY_OUT_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 1, MC_INITIALIZER_LIST(0), MC_INITIALIZER_LIST(), takes_imm)

#define X86_NULLARY_MACHINE_OP_MACRO_WRAPPER(macro, name, takes_imm) \
    macro(name, 0, MC_INITIALIZER_LIST(), MC_INITIALIZER_LIST(), takes_imm)

#define FOREACH_BINARY_MACHINE_OP(macro) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vaddps, false) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vsubps, false) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vmulps, false) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vdivps, false) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vpslld, true) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vpsrld, true) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vroundps, true) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vmaxps, false) \
    BINARY_MACHINE_OP_MACRO_WRAPPER(macro, vminps, false) \

#define FOREACH_X86_BINARY_MACHINE_OP(macro) \
    X86_BINARY_MACHINE_OP_MACRO_WRAPPER(macro, mov, true) \
    X86_BINARY_MACHINE_OP_MACRO_WRAPPER(macro, add, true) \
    X86_BINARY_MACHINE_OP_MACRO_WRAPPER(macro, sub, true) \
    X86_BINARY_MACHINE_OP_MACRO_WRAPPER(macro, and64, true) \

#define FOREACH_X86_UNARY_IN_MACHINE_OP(macro) \
    X86_UNARY_IN_MACHINE_OP_MACRO_WRAPPER(macro, push, false) \

#define FOREACH_X86_UNARY_OUT_MACHINE_OP(macro) \
    X86_UNARY_OUT_MACHINE_OP_MACRO_WRAPPER(macro, pop, false) \

#define FOREACH_X86_UNARY_MACHINE_OP(macro) \
    FOREACH_X86_UNARY_IN_MACHINE_OP(macro) \
    FOREACH_X86_UNARY_OUT_MACHINE_OP(macro) \

#define FOREACH_UNARY_MACHINE_OP(macro) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vmovaps, false) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vbroadcastss, false) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vsqrtps, false) \
    UNARY_MACHINE_OP_MACRO_WRAPPER(macro, vrsqrtps, false) \

#define FOREACH_X86_NULLARY_MACHINE_OP(macro) \
   X86_NULLARY_MACHINE_OP_MACRO_WRAPPER(macro, nop, false) \
   X86_NULLARY_MACHINE_OP_MACRO_WRAPPER(macro, ret, false) \

#define FOREACH_MACHINE_OP(macro) \
    FOREACH_UNARY_MACHINE_OP(macro) \
    FOREACH_BINARY_MACHINE_OP(macro) \
    FOREACH_X86_BINARY_MACHINE_OP(macro) \
    FOREACH_X86_UNARY_MACHINE_OP(macro) \
    FOREACH_X86_NULLARY_MACHINE_OP(macro)

// clang-format on

#define MACHINE_REGISTER_ENUM(register_name, register_number) register_name,
enum class Machine_Register { FOREACH_MACHINE_REGISTER(MACHINE_REGISTER_ENUM) };
#undef MACHINE_REGISTER_ENUM

uint64_t register_number(Machine_Register reg);

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

  bool operator==(const Memory_Reference &rhs) const {
    if (kind != rhs.kind)
      return false;

    switch (kind) {
    case Kind::Virtual:
      return virtual_reg() == rhs.virtual_reg();
    case Kind::Machine:
      return machine_reg() == rhs.machine_reg() && offset == rhs.offset;
    default:
      abort();
    }
  }
};

struct Immediate_Value {
  uint64_t value;

  Immediate_Value(uint64_t val) : value(val) {}
  Immediate_Value(uint32_t val) : value(uint64_t(val)) {}
  Immediate_Value(float val) : value(uint64_t(util::float_to_bits(val))) {}

  operator uint64_t() const { return value; }
  operator uint32_t() const { return uint32_t(value); }
  operator float() const { return float(util::bits_to_float(uint32_t(value))); }

  bool operator==(const Immediate_Value other) const {
    return value == other.value;
  }
};

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
  static Register Imm(unsigned long long imm) {
    return {Kind::Immediate, Immediate_Value(uint64_t(imm))};
  }
  static Register Imm(uint64_t imm) { return {Kind::Immediate, imm}; }
  static Register Imm(uint32_t imm) {
    return {Kind::Immediate, Immediate_Value(imm)};
  }
  static Register Imm(float imm) {
    return {Kind::Immediate, Immediate_Value(util::float_to_bits(imm))};
  }

  bool is_virtual() const { return kind == Kind::Virtual; }
  bool is_machine() const { return kind == Kind::Machine; }
  bool is_memory() const { return kind == Kind::Memory; }
  bool is_immediate() const { return kind == Kind::Immediate; }
  Virtual_Register virtual_reg() const {
    return std::get<Virtual_Register>(reg);
  }
  Virtual_Register &virtual_reg() { return std::get<Virtual_Register>(reg); }
  Machine_Register machine_reg() const {
    return std::get<Machine_Register>(reg);
  }
  Machine_Register &machine_reg() { return std::get<Machine_Register>(reg); }
  Memory_Reference memory_ref() const {
    return std::get<Memory_Reference>(reg);
  }
  Memory_Reference &memory_ref() { return std::get<Memory_Reference>(reg); }
  Immediate_Value imm() const { return std::get<Immediate_Value>(reg); }
  Immediate_Value &imm() { return std::get<Immediate_Value>(reg); }

  bool operator==(const Register &rhs) const {
    if (kind != rhs.kind)
      return false;

    switch (kind) {
    case Kind::Virtual:
      return virtual_reg() == rhs.virtual_reg();
    case Kind::Machine:
      return machine_reg() == rhs.machine_reg();
    case Kind::Memory:
      return memory_ref() == rhs.memory_ref();
    case Kind::Immediate:
      return imm() == rhs.imm();
    default:
      abort();
    }
  }

}; // namespace sdfjit::machinecode

struct Instruction {
  Op op;
  std::vector<Register> registers; // some of these are in-regs, some out-regs.
                                   // Each Op has different requirements :cry:

  std::vector<Register> set_registers() const;
  std::vector<Register> used_registers() const;
  bool sets(const Register &reg) const;
  bool uses(const Register &reg) const;
  bool can_use_immediates() const;

  void replace_register(const Register &from, const Register &to) {
    for (size_t i = 0; i < registers.size(); i++) {
      // XXX: we might need fancier things if we every do fancy effective-addrs
      // with virtual regs or want to replace machine regs
      if (registers[i] == from) {
        registers[i] = to;
      }
    }
  }

  void convert_to_nop() {
    op = Op::nop;
    registers.clear();
  }
};

struct Machine_Code {
  std::vector<Instruction> instructions{};
  Virtual_Register next_virtual_register{0};
  Constant_Pool constants{};
  Stack_Info stack_info{};

  static constexpr size_t constant_pool_arg_index = 3;

  static Machine_Code from_bytecode(const sdfjit::bytecode::Bytecode &bc);
  void resolve_immediates();

  Instruction &add_instruction(const Instruction &insn) {
    instructions.push_back(insn);
    return instructions.back();
  }

  Register new_virtual_register() {
    return Register{Register::Kind::Virtual, next_virtual_register++};
  }

  void allocate_registers();
  void add_prologue_and_epilogue();

#define UNARY_DECL(name, ...)                                                  \
  Register name(const Register &src);                                          \
  Register name(const Register &result, const Register &src);
#define X86_UNARY_DECL(name, ...) Register name(const Register &val);
#define BINARY_DECL(name, ...)                                                 \
  Register name(const Register &lhs, const Register &rhs);                     \
  Register name(const Register &result, const Register &lhs,                   \
                const Register &rhs);
#define X86_NULLARY_DECL(name, ...) Register name();

  FOREACH_UNARY_MACHINE_OP(UNARY_DECL);
  FOREACH_X86_UNARY_MACHINE_OP(X86_UNARY_DECL);
  FOREACH_BINARY_MACHINE_OP(BINARY_DECL);
  FOREACH_X86_NULLARY_MACHINE_OP(X86_NULLARY_DECL);

#undef X86_NULLARY_DECL
#undef BINARY_DECL
#undef X86_UNARY_DECL
#undef UNARY_DECL
};

std::ostream &operator<<(std::ostream &os, const Machine_Register reg);
std::ostream &operator<<(std::ostream &os, const Memory_Reference mem);
std::ostream &operator<<(std::ostream &os, const Immediate_Value imm);
std::ostream &operator<<(std::ostream &os, const Op op);
std::ostream &operator<<(std::ostream &os, const Register reg);
std::ostream &operator<<(std::ostream &os, const Instruction &insn);
std::ostream &operator<<(std::ostream &os, const Machine_Code &mc);

Register get_argument_register(size_t arg_index);

} // namespace sdfjit::machinecode
