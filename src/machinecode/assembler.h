#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#include "machinecode.h"

namespace sdfjit::machinecode {

struct Assembler {
  Machine_Code &mc;
  std::vector<uint8_t> buffer{};

  void assemble();
  void assemble_instruction(const Instruction &instruction);

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

#define DECLARE_OP_EMITTER(name, ...) void name(const Instruction &instruction);
  FOREACH_MACHINE_OP(DECLARE_OP_EMITTER);
};

std::ostream &operator<<(std::ostream &os, const Assembler &assembler);

} // namespace sdfjit::machinecode
