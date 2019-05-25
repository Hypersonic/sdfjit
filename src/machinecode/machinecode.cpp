#include "machinecode.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "insertion_set.h"
#include "registerallocator.h"

namespace sdfjit::machinecode {

uint64_t register_number(Machine_Register reg) {
#define MACHINE_REGISTER_NUMBER(register_name, register_number)                \
  case Machine_Register::register_name:                                        \
    return register_number;

  switch (reg) { FOREACH_MACHINE_REGISTER(MACHINE_REGISTER_NUMBER); }
  abort();
}

std::vector<Register> Instruction::set_registers() const {
#define GET_SET_REGISTER_IDXES(op_name, num_args, set_reg_idxes, ...)          \
  case Op::op_name: {                                                          \
    indexes = set_reg_idxes;                                                   \
    break;                                                                     \
  }

  std::vector<size_t> indexes;
  switch (op) { FOREACH_MACHINE_OP(GET_SET_REGISTER_IDXES); }

  std::vector<Register> result;
  result.reserve(indexes.size());
  for (const size_t index : indexes) {
    result.push_back(registers.at(index));
  }
  return result;

#undef GET_SET_REGISTER_IDXES
}

std::vector<Register> Instruction::used_registers() const {
#define GET_USED_REGISTER_IDXES(op_name, num_args, set_reg_idxes,              \
                                used_reg_idxes, ...)                           \
  case Op::op_name: {                                                          \
    indexes = used_reg_idxes;                                                  \
    break;                                                                     \
  }

  std::vector<size_t> indexes;
  switch (op) { FOREACH_MACHINE_OP(GET_USED_REGISTER_IDXES); }

  std::vector<Register> result;
  result.reserve(indexes.size());
  for (const size_t index : indexes) {
    result.push_back(registers.at(index));
  }
  return result;

#undef GET_SET_REGISTER_IDXES
}

bool Instruction::sets(const Register &reg) const {
  auto set_regs = set_registers();
  return std::find(set_regs.begin(), set_regs.end(), reg) != set_regs.end();
}

bool Instruction::uses(const Register &reg) const {
  auto used_regs = used_registers();
  return std::find(used_regs.begin(), used_regs.end(), reg) != used_regs.end();
}

bool Instruction::can_use_immediates() const {
#define USES_IMMEDIATE(op_name, num_args, set_reg_idxes, used_reg_idxes,       \
                       takes_imm, ...)                                         \
  case Op::op_name:                                                            \
    return takes_imm;

  switch (op) { FOREACH_MACHINE_OP(USES_IMMEDIATE); }
  abort();
#undef USES_IMMEDIATE
}

bool Instruction::can_use_memory_ref() const {
#define USES_MEMORY(op_name, num_args, set_reg_idxes, used_reg_idxes,          \
                    takes_imm, takes_mem...)                                   \
  case Op::op_name:                                                            \
    return takes_mem;

  switch (op) { FOREACH_MACHINE_OP(USES_MEMORY); }
  abort();
#undef USES_MEMORY
}

Machine_Code Machine_Code::from_bytecode(const sdfjit::bytecode::Bytecode &bc) {
  Machine_Code mc{};

  // mapping of bytecode nodes to the machine-code register that
  // contains it's results
  // XXX: maybe this is a bit inflexible? maybe we want a list of result
  // locations, or even just a list of result registers in some agreed-upon
  // order. For now we're doing single-out instructions so this is fine.
  std::unordered_map<sdfjit::bytecode::Node_Id, Register> bc_to_reg{};

  for (size_t id = 0; id < bc.nodes.size(); id++) {
    const auto &node = bc.nodes[id];

    switch (node.op) {
    case sdfjit::bytecode::Op::Nop: {
      break;
    }

    case sdfjit::bytecode::Op::Load_Arg: {
      auto result = mc.vmovaps(get_argument_register(node.arg_index));
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Store_Result: {
      // XXX: this is written super poorly. Need to clean it up
      auto result = Register::Memory(
          get_argument_register(4).memory_ref().machine_reg(), 0);
      mc.vmovaps(result, bc_to_reg.at(node.arguments.at(0)));
      break;
    }

    case sdfjit::bytecode::Op::Assign:
      abort(); // TODO

    case sdfjit::bytecode::Op::Assign_Float: {
      auto result = mc.vbroadcastss(Register::Imm(node.value));
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Add: {
      auto lhs = bc_to_reg.at(node.arguments.at(0));
      auto rhs = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.vaddps(lhs, rhs);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Subtract: {
      auto lhs = bc_to_reg.at(node.arguments.at(0));
      auto rhs = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.vsubps(lhs, rhs);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Multiply: {
      auto lhs = bc_to_reg.at(node.arguments.at(0));
      auto rhs = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.vmulps(lhs, rhs);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Divide:
      abort(); // TODO

    case sdfjit::bytecode::Op::Sqrt: {
      auto src = bc_to_reg.at(node.arguments.at(0));
      auto result = mc.vsqrtps(src);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Rsqrt:
      abort(); // TODO

    case sdfjit::bytecode::Op::Abs: {
      // there's a good breakdown of options in an answer here:
      // https://stackoverflow.com/questions/32408665/fastest-way-to-compute-absolute-value-using-sse
      // We choose option 4, which is to shift left by one and then right by
      // one. We might want to revisit this, but it seemed like a decent option
      // given that our register allocator will probably not be extremely good,
      // and this doesn't use an additional register.
      auto src = bc_to_reg.at(node.arguments.at(0));
      auto result =
          mc.vpsrld(mc.vpslld(src, Register::Imm(1u)), Register::Imm(1u));
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Negate: {
      auto val = bc_to_reg.at(node.arguments.at(0));
      auto result =
          mc.vxorps(mc.vbroadcastss(Register::Imm(uint32_t(0x80000000))), val);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Min: {
      auto lhs = bc_to_reg.at(node.arguments.at(0));
      auto rhs = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.vminps(lhs, rhs);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Max: {
      auto lhs = bc_to_reg.at(node.arguments.at(0));
      auto rhs = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.vmaxps(lhs, rhs);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Sin: {
      auto x = bc_to_reg.at(node.arguments.at(0));
      auto result = mc.sin(x);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Cos: {
      auto x = bc_to_reg.at(node.arguments.at(0));
      auto result = mc.cos(x);
      bc_to_reg[id] = result;
      break;
    }

    case sdfjit::bytecode::Op::Mod: {
      auto x = bc_to_reg.at(node.arguments.at(0));
      auto m = bc_to_reg.at(node.arguments.at(1));
      auto result = mc.mod(x, m);
      bc_to_reg[id] = result;
      break;
    }
    }
  }

  return mc;
}

void Machine_Code::resolve_immediates() {
  for (size_t i = 0; i < instructions.size(); i++) {
    if (instructions[i].can_use_immediates())
      continue;

    for (auto &reg : instructions[i].registers) {
      if (!reg.is_immediate())
        continue;

      // add constant to the pool
      size_t constant_offset = constants.add(uint32_t(reg.imm()));

      // update the register to be a memory
      // reference
      // XXX: do we want to make an api to
      // make this much cleaner?
      //      if we find ourselves rewriting
      //      registers a lot it might be
      //      good...
      reg.kind = Register::Kind::Memory;
      reg.reg = get_argument_register(constant_pool_arg_index).reg;
      reg.memory_ref().offset = constant_offset;
    }
  }
}

void Machine_Code::allocate_registers() {
  Linear_Scan_Register_Allocator lsra{};
  lsra.allocate(*this);
}

void Machine_Code::add_prologue_and_epilogue() {
  Insertion_Set insertions{*this};

  // prologue:
  // push rbp
  // mov rbp, rsp
  // sub rsp, <stack_size>
  // and rsp, 0xffffffffffffffe0
  insertions.before.push(0, Register::Machine(Machine_Register::rbp));
  insertions.before.mov(0, Register::Machine(Machine_Register::rbp),
                        Register::Machine(Machine_Register::rsp));
  insertions.before.sub(0, Register::Machine(Machine_Register::rsp),
                        Register::Imm(stack_info.current_offset));
  insertions.before.and64(0, Register::Machine(Machine_Register::rsp),
                          Register::Imm(0xffffffffffffffe0ull));

  // epilogue:
  // mov rsp, rbp
  // pop rbp
  // ret
  insertions.after.mov(instructions.size() - 1,
                       Register::Machine(Machine_Register::rsp),
                       Register::Machine(Machine_Register::rbp));
  insertions.after.pop(instructions.size() - 1,
                       Register::Machine(Machine_Register::rbp));
  insertions.after.ret(instructions.size() - 1);

  insertions.commit();
}

#define DEFINE_UNARY_OP(name, ...)                                             \
  Register Machine_Code::name(const Register &src) {                           \
    return name(new_virtual_register(), src);                                  \
  }                                                                            \
  Register Machine_Code::name(const Register &result, const Register &src) {   \
    return add_instruction(Instruction{Op::name, {result, src}})               \
        .set_registers()                                                       \
        .at(0);                                                                \
  }

#define DEFINE_X86_UNARY_OP(name, ...)                                         \
  Register Machine_Code::name(const Register &val) {                           \
    return add_instruction(Instruction{Op::name, {val}})                       \
        .set_registers()                                                       \
        .at(0);                                                                \
  }

#define DEFINE_BINARY_OP(name, ...)                                            \
  Register Machine_Code::name(const Register &lhs, const Register &rhs) {      \
    return name(new_virtual_register(), lhs, rhs);                             \
  }                                                                            \
  Register Machine_Code::name(const Register &result, const Register &lhs,     \
                              const Register &rhs) {                           \
    return add_instruction(Instruction{Op::name, {result, lhs, rhs}})          \
        .set_registers()                                                       \
        .at(0);                                                                \
  }

FOREACH_UNARY_MACHINE_OP(DEFINE_UNARY_OP);
FOREACH_X86_UNARY_MACHINE_OP(DEFINE_X86_UNARY_OP);
FOREACH_BINARY_MACHINE_OP(DEFINE_BINARY_OP);

Register Machine_Code::mod(const Register &lhs, const Register &rhs) {
  /* x' = x % m:
   *
   * d = trunc(x / m);
   * x' = x - (d * m);
   */

  // rounding mode = 0b11 (truncate towards zero)
  auto &x = lhs;
  auto &m = rhs;
  auto d = vroundps(vdivps(x, m), Register::Imm(uint32_t(0b11)));
  auto result = vsubps(x, vmulps(d, m));
  return result;
}

Register Machine_Code::cos(const Register &val) {
  return sin(vaddps(val, vbroadcastss(Register::Imm(float(M_PI / 2)))));
}

Register Machine_Code::sin(const Register &val) {
  /* we compute sin with Bhaskara I's formula, see
   * https://en.wikipedia.org/wiki/Bhaskara_I%27s_sine_approximation_formula
   * this isn't *super* accurate, but it's probably good enough for now,
   * and it doesn't take very many instructions to compute
   *
   * sin(x) = (16 * x * (pi - x)) / (5 * pi^2 - 4 * x * (pi - x))
   *
   * This sine approximation is only accurate in [0, M_PI], so we need to modulo
   * down to that range. However, since sine makes sense in [0, 2*M_PI], we
   * actually want to bring it there, and then subtract M_PI to bring it into
   * valid range, keep track of the sign bit and mask it off for our operation.
   * Then we can flip the sign of the result if our sign is positive.
   */
  auto pi = vbroadcastss(Register::Imm(float(M_PI)));
  auto pi_times_two = vbroadcastss(Register::Imm(float(M_PI * 2)));

  auto x = mod(val, pi_times_two);
  x = vsubps(x, pi);
  auto saved_sign =
      vandps(x, vbroadcastss(Register::Imm(uint32_t(0x80000000))));
  x = vandps(x, vbroadcastss(Register::Imm(uint32_t(0x7fffffff))));

  auto five_time_pi_squared =
      vbroadcastss(Register::Imm(float(5 * M_PI * M_PI)));
  auto pi_minus_x = vsubps(pi, x);
  auto four = vbroadcastss(Register::Imm(4.0f));
  auto four_time_x_times_pi_minus_x = vmulps(four, vmulps(x, pi_minus_x));

  auto numerator = vmulps(four, four_time_x_times_pi_minus_x);
  auto denominator = vsubps(five_time_pi_squared, four_time_x_times_pi_minus_x);
  auto unsigned_sine = vdivps(numerator, denominator);

  // add back in the sign bit we saved off earlier:
  auto result = vxorps(
      unsigned_sine,
      vxorps(saved_sign, vbroadcastss(Register::Imm(uint32_t(0x80000000)))));

  return result;
}

std::ostream &operator<<(std::ostream &os, Machine_Register reg) {
#define MACHINE_REGISTER_TOSTRING(register_name, register_number)              \
  case Machine_Register::register_name:                                        \
    return os << #register_name;
  switch (reg) { FOREACH_MACHINE_REGISTER(MACHINE_REGISTER_TOSTRING); }
  abort();
#undef MACHINE_REGISTER_TOSTRING
}

std::ostream &operator<<(std::ostream &os, const Op op) {
#define MACHINE_OP_TOSTRING(op_name, ...)                                      \
  case Op::op_name:                                                            \
    return os << #op_name;
  switch (op) { FOREACH_MACHINE_OP(MACHINE_OP_TOSTRING); }
  abort();
#undef MACHINE_OP_TOSTRING
}

std::ostream &operator<<(std::ostream &os, const Memory_Reference mem) {
  if (mem.is_virtual()) {
    return os << '[' << mem.virtual_reg() << " + " << mem.offset << ']';
  } else if (mem.is_machine()) {
    return os << '[' << mem.machine_reg() << " + " << mem.offset << ']';
  }
  abort();
}

std::ostream &operator<<(std::ostream &os, const Immediate_Value imm) {
  return os << uint64_t(imm);
}

std::ostream &operator<<(std::ostream &os, const Register reg) {
  if (reg.is_virtual()) {
    return os << '@' << reg.virtual_reg();
  } else if (reg.is_machine()) {
    return os << reg.machine_reg();
  } else if (reg.is_memory()) {
    return os << reg.memory_ref();
  } else if (reg.is_immediate()) {
    return os << '$' << reg.imm();
  }
  abort();
}

std::ostream &operator<<(std::ostream &os, const Instruction &insn) {
  os << insn.op << ' ';
  for (const auto reg : insn.registers) {
    os << reg << ", ";
  }
  return os;
}

std::ostream &operator<<(std::ostream &os, const Machine_Code &mc) {
  for (const auto &insn : mc.instructions) {
    os << insn << std::endl;
  }
  return os;
}

Register get_argument_register(size_t arg_index) {
  Machine_Register reg;
  switch (arg_index) {
  case 0:
    reg = Machine_Register::rdi;
    break;
  case 1:
    reg = Machine_Register::rsi;
    break;
  case 2:
    reg = Machine_Register::rdx;
    break;
  case 3:
    reg = Machine_Register::rcx;
    break;
  case 4:
    reg = Machine_Register::r8;
    break;
  case 5:
    reg = Machine_Register::r9;
    break;
  default:
    abort();
  }
  return Register::Memory(reg, 0);
}

} // namespace sdfjit::machinecode
