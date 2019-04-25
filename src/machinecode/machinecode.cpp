#include "machinecode.h"

#include <unordered_map>

namespace sdfjit::machinecode {

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
    case sdfjit::bytecode::Op::Load_Arg: {
      auto result = mc.movaps(get_argument_register(node.arg_index));
      bc_to_reg[id] = result;
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

    case sdfjit::bytecode::Op::Negate:
      abort(); // TODO

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
    }
  }

  return mc;
}

#define DEFINE_UNARY_OP(name, ...)                                             \
  Register Machine_Code::name(const Register &src) {                           \
    return add_instruction(                                                    \
               Instruction{Op::name, {new_virtual_register(), src}})           \
        .set_registers()                                                       \
        .at(0);                                                                \
  }

#define DEFINE_BINARY_OP(name, ...)                                            \
  Register Machine_Code::name(const Register &lhs, const Register &rhs) {      \
    return add_instruction(                                                    \
               Instruction{Op::name, {new_virtual_register(), lhs, rhs}})      \
        .set_registers()                                                       \
        .at(0);                                                                \
  }

FOREACH_UNARY_MACHINE_OP(DEFINE_UNARY_OP);
FOREACH_BINARY_MACHINE_OP(DEFINE_BINARY_OP);

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
