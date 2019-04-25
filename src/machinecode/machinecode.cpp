#include "machinecode.h"

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

Register Machine_Code::movaps(const Register &src) {
  return add_instruction(Instruction{Op::movaps, {new_virtual_register(), src}})
      .set_registers()
      .at(0);
}

Register Machine_Code::vaddps(const Register &lhs, const Register &rhs) {
  return add_instruction(
             Instruction{Op::vaddps, {new_virtual_register(), lhs, rhs}})
      .set_registers()
      .at(0);
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

std::ostream &operator<<(std::ostream &os, const Register reg) {
  if (reg.is_virtual()) {
    return os << '@' << reg.virtual_reg();
  } else if (reg.is_machine()) {
    return os << reg.machine_reg();
  } else if (reg.is_memory()) {
    return os << reg.memory_ref();
  }
  abort();
}

std::ostream &operator<<(std::ostream &os, const Instruction &insn) {
  os << insn.op << '(';
  for (const auto reg : insn.registers) {
    os << reg << ", ";
  }
  return os << ')';
}

std::ostream &operator<<(std::ostream &os, const Machine_Code &mc) {
  for (const auto &insn : mc.instructions) {
    os << insn << std::endl;
  }
  return os;
}

} // namespace sdfjit::machinecode
