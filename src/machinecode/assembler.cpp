#include "assembler.h"

#include "util/hexdump.h"
#include "util/view.h"

namespace sdfjit::machinecode {

void Assembler::assemble() {
  for (const auto &instruction : mc.instructions) {
    assemble_instruction(instruction);
  }
}

void Assembler::assemble_instruction(const Instruction &instruction) {
#define ASSEMBLE_OP(name, ...)                                                 \
  case Op::name: {                                                             \
    name(instruction);                                                         \
    break;                                                                     \
  };

  size_t begin = buffer.size();
  switch (instruction.op) { FOREACH_MACHINE_OP(ASSEMBLE_OP); }
  size_t end = buffer.size();

  mark_instruction(begin, end - begin);
}

void Assembler::vminps(const Instruction &instruction) {
  auto r1 = register_number(instruction.registers.at(0).machine_reg());
  auto r2 = register_number(instruction.registers.at(1).machine_reg());
  auto r3 = register_number(instruction.registers.at(2).machine_reg());

  emit_byte(0xc5);
  emit_byte(0x80 | ((~r2 & 0xf) << 3) | 0x4);
  emit_byte(0x5d);
  emit_byte(0xc0 | (r1 << 3) | r3);
}

void Assembler::vmaxps(const Instruction &instruction) {
  auto r1 = register_number(instruction.registers.at(0).machine_reg());
  auto r2 = register_number(instruction.registers.at(1).machine_reg());
  auto r3 = register_number(instruction.registers.at(2).machine_reg());

  emit_byte(0xc5);
  emit_byte(0x80 | ((~r2 & 0xf) << 3) | 0x4);
  emit_byte(0x5f);
  emit_byte(0xc0 | (r1 << 3) | r3);
}

void Assembler::vmovaps(const Instruction &instruction) {
  auto &lhs = instruction.registers.at(0);
  auto &rhs = instruction.registers.at(1);

  Machine_Register reg;
  Memory_Reference mem;

  emit_byte(0xc5);
  emit_byte(0xfc);

  if (lhs.is_machine() && rhs.is_memory()) {
    // vmovaps reg, [memory_location]
    emit_byte(0x28);
    reg = lhs.machine_reg();
    mem = rhs.memory_ref();
  } else if (lhs.is_memory() && rhs.is_machine()) {
    // vmovaps [memory_location], reg
    emit_byte(0x29);
    reg = rhs.machine_reg();
    mem = lhs.memory_ref();
  } else {
    // ???
    abort();
  }

  // the encoding is the same for loads and stores
  // (arg1 = register, arg2 = memory)
  //
  if (mem.offset == 0) {
    // offsets of zero don't need an immediate

    // mod r/m:
    emit_byte((register_number(reg) << 3) | register_number(mem.machine_reg()));
    // SIB, only for some registers:
    // XXX: technically more than rsp are required, but we don't use them.
    //      Maybe eventually we want to support them
    if (mem.machine_reg() == Machine_Register::rsp) {
      emit_byte((register_number(mem.machine_reg()) << 3) |
                register_number(mem.machine_reg()));
    }
  } else if (mem.offset < 0x80) {
    // offsets of less than 0x80 can use a 1-byte immediate

    // mod r/m:
    emit_byte(0x40 | (register_number(reg) << 3) |
              register_number(mem.machine_reg()));

    // SIB, only for some registers:
    // XXX: technically more than rsp are required, but we don't use them.
    //      Maybe eventually we want to support them
    if (mem.machine_reg() == Machine_Register::rsp) {
      emit_byte((register_number(mem.machine_reg()) << 3) |
                register_number(mem.machine_reg()));
    }

    // imm:
    emit_byte(mem.offset);
  } else if (mem.offset < 0xffffffff) {
    // offsets in this range use a dword immediate

    // mod r/m:
    emit_byte(0x80 | (register_number(reg) << 3) |
              register_number(mem.machine_reg()));

    // SIB, only for some registers:
    // XXX: technically more than rsp are required, but we don't use them.
    //      Maybe eventually we want to support them
    if (mem.machine_reg() == Machine_Register::rsp) {
      emit_byte((register_number(mem.machine_reg()) << 3) |
                register_number(mem.machine_reg()));
    }

    // imm:
    emit_dword(mem.offset);
  } else {
    std::cout << "unhandled offset size: " << mem.offset
              << " in instruction: " << instruction << std::endl;
    abort();
  }
}

void Assembler::vbroadcastss(const Instruction &instruction) {
  auto dst = instruction.registers.at(0).machine_reg();
  auto src = instruction.registers.at(1).memory_ref();

  emit_byte(0xc4);
  emit_byte(0xe2);
  emit_byte(0x7d);
  emit_byte(0x18);

  // XXX: we only handle broadcasts of constants here, which are all stored
  //      offset from rcx, so assert that we're only using rcx
  if (src.machine_reg() != Machine_Register::rcx) {
    abort();
  }

  if (src.offset == 0) {
    // zeros can be done with
    emit_byte((register_number(dst) << 3) | register_number(src.machine_reg()));
  } else if (src.offset < 0x80) {
    emit_byte(0x40 | (register_number(dst) << 3) |
              register_number(src.machine_reg()));
    emit_byte(src.offset);
  } else {
    std::cerr << "Unexpected offset for vbroadcastss: " << src.offset
              << std::endl;
    abort();
  }
}

void Assembler::vsqrtps(const Instruction &instruction) {
  auto dst = register_number(instruction.registers.at(0).machine_reg());
  auto src = register_number(instruction.registers.at(1).machine_reg());

  emit_byte(0xc5);
  emit_byte(0xfc);
  emit_byte(0x51);
  emit_byte(0xc0 | (dst << 3) | src);
}

void Assembler::vaddps(const Instruction &instruction) {
  auto r1 = register_number(instruction.registers.at(0).machine_reg());
  auto r2 = register_number(instruction.registers.at(1).machine_reg());
  auto r3 = register_number(instruction.registers.at(2).machine_reg());

  emit_byte(0xc5);
  emit_byte(0x80 | ((~r2 & 0xf) << 3) | 0x4);
  emit_byte(0x58);
  emit_byte(0xc0 | (r1 << 3) | r3);
}

void Assembler::vsubps(const Instruction &instruction) {
  auto r1 = register_number(instruction.registers.at(0).machine_reg());
  auto r2 = register_number(instruction.registers.at(1).machine_reg());
  auto r3 = register_number(instruction.registers.at(2).machine_reg());

  emit_byte(0xc5);
  emit_byte(0x80 | ((~r2 & 0xf) << 3) | 0x4);
  emit_byte(0x5C);
  emit_byte(0xc0 | (r1 << 3) | r3);
}

void Assembler::vmulps(const Instruction &instruction) {
  auto r1 = register_number(instruction.registers.at(0).machine_reg());
  auto r2 = register_number(instruction.registers.at(1).machine_reg());
  auto r3 = register_number(instruction.registers.at(2).machine_reg());

  emit_byte(0xc5);
  emit_byte(0x80 | ((~r2 & 0xf) << 3) | 0x4);
  emit_byte(0x59);
  emit_byte(0xc0 | (r1 << 3) | r3);
}

void Assembler::vpslld(const Instruction &instruction) {
  auto dst = register_number(instruction.registers.at(0).machine_reg());
  auto src = register_number(instruction.registers.at(1).machine_reg());
  auto imm = instruction.registers.at(2).imm();

  if (imm > 0xff) {
    std::cerr << "Immediate to vpslld is too large: " << imm << std::endl;
    abort();
  }

  emit_byte(0xc5);
  emit_byte(0x80 | ((~dst & 0xf) << 3) | 0x5);
  emit_byte(0x72);
  emit_byte(0xf0 | src);
  emit_byte(imm);
}

void Assembler::vpsrld(const Instruction &instruction) {
  auto dst = register_number(instruction.registers.at(0).machine_reg());
  auto src = register_number(instruction.registers.at(1).machine_reg());
  auto imm = instruction.registers.at(2).imm();

  if (imm > 0xff) {
    std::cerr << "Immediate to vpsrld is too large: " << imm << std::endl;
    abort();
  }

  emit_byte(0xc5);
  emit_byte(0x80 | ((~dst & 0xf) << 3) | 0x5);
  emit_byte(0x72);
  emit_byte(0xd0 | src);
  emit_byte(imm);
}

void Assembler::pop(const Instruction &instruction) {
  auto reg = register_number(instruction.registers.at(0).machine_reg());
  emit_byte(0x58 | reg);
}

void Assembler::push(const Instruction &instruction) {
  auto reg = register_number(instruction.registers.at(0).machine_reg());
  emit_byte(0x50 | reg);
}

void Assembler::ret(const Instruction &instruction) {
  (void)instruction;
  emit_byte(0xc3);
}

void Assembler::add(const Instruction &instruction) {
  auto dst = instruction.registers.at(0);
  auto src = instruction.registers.at(1);

  if (dst.is_machine() && src.is_immediate()) {
    if (dst.machine_reg() != Machine_Register::rsp) {
      // we only need this for rsp and I'd rather just fast-fail if we don't
      // have that and deal with figuring out whether it encodes the same until
      // then.
      abort();
    }

    emit_byte(0x48);
    auto imm = src.imm();
    if (imm <= 0x7f) {
      emit_byte(0x83);
      emit_byte(0xc0 | register_number(dst.machine_reg()));
      emit_byte(imm);
    } else if (imm <= 0xffffffff) {
      emit_byte(0x81);
      emit_byte(0xc0 | register_number(dst.machine_reg()));
      emit_dword(imm);
    } else {
      abort();
    }
  } else {
    // we don't support other kinds right now because i am lazy
    abort();
  }
}

void Assembler::sub(const Instruction &instruction) {
  auto dst = instruction.registers.at(0);
  auto src = instruction.registers.at(1);

  if (dst.is_machine() && src.is_immediate()) {
    if (dst.machine_reg() != Machine_Register::rsp) {
      // we only need this for rsp and I'd rather just fast-fail if we don't
      // have that and deal with figuring out whether it encodes the same until
      // then.
      abort();
    }

    emit_byte(0x48);
    auto imm = src.imm();
    if (imm <= 0x7f) {
      emit_byte(0x83);
      emit_byte(0xe8 | register_number(dst.machine_reg()));
      emit_byte(imm);
    } else if (imm <= 0xffffffff) {
      emit_byte(0x81);
      emit_byte(0xe8 | register_number(dst.machine_reg()));
      emit_dword(imm);
    } else {
      abort();
    }
  } else {
    // we don't support other kinds right now because i am lazy
    abort();
  }
}

void Assembler::mov(const Instruction &instruction) {
  auto dst = instruction.registers.at(0);
  auto src = instruction.registers.at(1);

  if (src.is_machine() && dst.is_machine()) {
    emit_byte(0x48);
    emit_byte(0x89);
    emit_byte(0xc0 | (register_number(src.machine_reg()) << 3) |
              register_number(dst.machine_reg()));
  } else {
    // we don't handle this right now
    abort();
  }
}

std::ostream &operator<<(std::ostream &os, const Assembler &assembler) {
  for (size_t i = 0; i < assembler.instruction_offsets_and_sizes.size(); i++) {
    const auto &instruction = assembler.mc.instructions.at(i);
    const auto [offset, size] = assembler.instruction_offsets_and_sizes.at(i);
    auto view = util::vector_view<uint8_t>{assembler.buffer, offset, size};
    os << instruction << std::endl;
    util::hexdump(os, view);
  }
  return os;
}

} // namespace sdfjit::machinecode
