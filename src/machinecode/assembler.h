#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "machinecode.h"

namespace sdfjit::machinecode {

struct Assembler {
  Machine_Code &mc;
  std::vector<uint8_t> buffer{};
  // offsets & lengths into the buffer for instructions
  // this is mostly useful for dumping out assembled bytes
  std::vector<std::pair<size_t, size_t>> instruction_offsets_and_sizes{};

  void assemble();
  void assemble_instruction(const Instruction &instruction);

  void mark_instruction(size_t offset, size_t length) {
    instruction_offsets_and_sizes.push_back({offset, length});
  }

  void emit_byte(uint8_t val) { buffer.push_back(val); }
  void emit_word(uint16_t val) {
    emit_byte(val & 0xff);
    emit_byte(val >> 8);
  }
  void emit_dword(uint32_t val) {
    emit_word(val & 0xffff);
    emit_word(val >> 16);
  }
  void emit_qword(uint64_t val) {
    emit_dword(val & 0xffffffff);
    emit_dword(val >> 32);
  }

  template <uint8_t opcode>
  void binary_op(const Register &r1, const Register &r2, const Register &r3) {
    auto rn1 = register_number(r1.machine_reg());
    auto rn2 = register_number(r2.machine_reg());
    emit_byte(0xc5);
    emit_byte(0x80 | ((~rn2 & 0xf) << 3) | 0x4);
    emit_byte(opcode);

    if (r3.is_machine()) {
      auto rn3 = register_number(r3.machine_reg());
      emit_byte(0xc0 | (rn1 << 3) | rn3);
    } else {
      auto rr3 = r3.memory_ref();
      if (rr3.machine_reg() != Machine_Register::rsp) {
        std::cerr << "Error: cannot handle non-rsp registers in binary_op"
                  << std::endl;
        abort();
      }

      auto rn3 = register_number(rr3.machine_reg());

      emit_byte(0x80 | (rn1 << 3) | rn3);
      emit_byte((rn3 << 3) | rn3);
      emit_dword(rr3.offset);
    }
  }

#define DECLARE_OP_EMITTER(name, ...) void name(const Instruction &instruction);
  FOREACH_MACHINE_OP(DECLARE_OP_EMITTER);
};

std::ostream &operator<<(std::ostream &os, const Assembler &assembler);

} // namespace sdfjit::machinecode
