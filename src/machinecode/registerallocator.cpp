#include "registerallocator.h"

#include <vector>

#include "insertion_set.h"
#include "machinecode.h"

namespace sdfjit::machinecode {

void Linear_Scan_Register_Allocator::allocate(Machine_Code &mc) {
  // machine regs that are available for allocation
  std::vector<Machine_Register> machine_regs_available(machine_registers);
  // temporary regs that are available for allocation
  // (reset at the beginning of each instruction)
  std::vector<Machine_Register> temp_regs_available;
  // what registers we've allocated for each virtual register
  std::unordered_map<Virtual_Register, Register> allocated_values{};

  Insertion_Set insertion_set{mc};

  std::vector<Virtual_Register> pending{};
  std::vector<Virtual_Register> in_use{};
  std::vector<Virtual_Register> dead{};

  compute_live_intervals(mc);

  // I don't think this is textbook linear-scan, but it was a thing that made
  // sense to me. basically, we scan through the program, keeping 3 lists:
  //    - pending
  //        - all virtual registers start here.
  //        - when we reach the register's birth, we select either a machine
  //          register, if one is available, or allocate a stack slot for it
  //          otherwise, and move it to the `in-use` list
  //    - in-use
  //        - while we're inside the register's live range, it stays here
  //        - as we iterate over instructions, replace uses of the virtual
  //          registers in this list with their allocated versions.
  //        - once we hit the register's death, move it to the `dead` list
  //    - dead
  //        - registers we've finished allocating end up here
  //
  // after we've done this, we need to insert stack loads/stores for spilled
  // registers around their uses,  but that's a second pass

  pending.reserve(mc.instructions.size());
  in_use.reserve(mc.instructions.size());
  dead.reserve(mc.instructions.size());

  for (const auto &insn : mc.instructions) {
    for (const auto &reg : insn.set_registers()) {
      if (reg.is_virtual()) {
        pending.push_back(reg.virtual_reg());
      }
    }
  }

  // TODO: at some point refactor some of these into methods
  auto remove_from_pending = [&](Register reg) {
    pending.erase(
        std::remove(pending.begin(), pending.end(), reg.virtual_reg()));
  };
  auto remove_from_in_use = [&](Register reg) {
    in_use.erase(std::remove(in_use.begin(), in_use.end(), reg.virtual_reg()));
  };
  auto add_to_in_use = [&](Register reg) {
    in_use.push_back(reg.virtual_reg());
  };
  auto add_to_dead = [&](Register reg) {
    if (allocated_values.at(reg.virtual_reg()).is_machine()) {
      machine_regs_available.push_back(
          allocated_values.at(reg.virtual_reg()).machine_reg());
    }
    dead.push_back(reg.virtual_reg());
  };
  auto register_is_born_at = [&](Register reg, size_t time) -> bool {
    return live_intervals.at(reg).first == time;
  };
  auto register_dies_at = [&](Register reg, size_t time) -> bool {
    return live_intervals.at(reg).last == time;
  };
  auto materialize_register_at = [&](Register reg, size_t instruction_idx) {
    auto alloc_reg = allocated_values.at(reg.virtual_reg());
    auto &insn = mc.instructions[instruction_idx];

    if (alloc_reg.is_machine()) {
      // it's just a register, we can just put it in
      insn.replace_register(reg, alloc_reg);
    } else {
      // it's a spilled register, so we need to insert a load from a temp reg
      // before and a store back after
      auto temp_reg = Register::Machine(temp_regs_available.back());
      temp_regs_available.pop_back();
      std::cout << "using temp reg: " << temp_reg << std::endl;

      if (insn.uses(reg)) {
        // load to temp reg
        insertion_set.before.vmovaps(instruction_idx, temp_reg, alloc_reg);
      }

      if (insn.sets(reg)) {
        // store from temp reg
        insertion_set.after.vmovaps(instruction_idx, alloc_reg, temp_reg);
      }

      // do op on temp reg
      insn.replace_register(reg, temp_reg);
    }
  };
  auto assign_slot = [&](Register reg) -> Register {
    Register assigned;
    if (machine_regs_available.empty()) {
      // XXX: dont' hardcode this size, grab it from mc
      auto stack_slot = mc.stack_info.add_slot(256 / 8);
      assigned = Register::Memory(Machine_Register::rsp, stack_slot);
    } else {
      assigned = Register::Machine(machine_regs_available.back());
      machine_regs_available.pop_back();
    }
    allocated_values[reg.virtual_reg()] = assigned;
    return assigned;
  };

  for (size_t i = 0; i < mc.instructions.size(); i++) {
    const auto &insn = mc.instructions[i];
    temp_regs_available = temp_regs; // reset temp regs

    for (const auto &reg : insn.registers) {
      // we're only worried about virtual registers
      if (!reg.is_virtual())
        continue;

      if (register_is_born_at(reg, i)) {
        // pending -> in_use
        remove_from_pending(reg);
        add_to_in_use(reg);
        assign_slot(reg);
      } else if (register_dies_at(reg, i)) {
        // in_use -> dead
        remove_from_in_use(reg);
        add_to_dead(reg);
      }

      // always materialize the virtual register into a concrete one
      materialize_register_at(reg, i);
    }
  }

  // commit in our loads/stores of spilled registers
  insertion_set.commit();
}

void Linear_Scan_Register_Allocator::compute_live_intervals(Machine_Code &mc) {
  for (size_t i = 0; i < mc.instructions.size(); i++) {
    auto &insn = mc.instructions[i];
    for (auto &reg : insn.registers) {
      if (!reg.is_virtual())
        continue;

      if (live_intervals.contains(reg)) {
        live_intervals.at(reg).last = i;
      } else {
        auto &interval = live_intervals[reg];
        interval.first = interval.last = i;
      }
    }
  }
}

} // namespace sdfjit::machinecode
