#include "insertion_set.h"

#include <algorithm>

namespace sdfjit::machinecode {

void Inserter::add_instruction(size_t index, const Instruction &instruction) {
  set.insertions.push_back({side, index, instruction});
}

void Insertion_Set::commit() {
  // sort insertions in reverse order that they'll be put in the program
  // (that is, last to first, Afters come before Begins for the same index)
  std::sort(insertions.begin(), insertions.end(),
            [](const Insert_At &a, const Insert_At &b) {
              if (a.index == b.index) {
                static_assert(uint32_t(Insert_Side::Before) <
                              uint32_t(Insert_Side::After));
                return a.side > b.side;
              }
              return a.index > b.index;
            });

  mc.instructions.reserve(mc.instructions.size() + insertions.size());

  for (const auto &insertion : insertions) {
    size_t insert_index;
    if (insertion.side == Insert_Side::Before) {
      insert_index = insertion.index;
    } else {
      insert_index = insertion.index + 1;
    }

    mc.instructions.insert(mc.instructions.begin() + insert_index,
                           insertion.instruction);
  }
}

} // namespace sdfjit::machinecode
