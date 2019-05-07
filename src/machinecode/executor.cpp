#include "executor.h"

#include <cstring>
#include <sys/mman.h>

#include "assembler.h"
#include "machinecode.h"

namespace sdfjit::machinecode {

void Executor::create() {
  static constexpr size_t PAGE_SIZE = 0x1000;

  auto round_to_next_page_size = [](size_t size) -> size_t {
    return (size & ~(PAGE_SIZE - 1)) + PAGE_SIZE;
  };
  auto create_region = [&round_to_next_page_size](size_t size) -> void * {
    // regions are initially RW, and we'll remap them to RX once we're done
    // filling them
    return mmap(NULL, round_to_next_page_size(size), PROT_READ | PROT_WRITE,
                MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  };
  auto finalize_region = [&round_to_next_page_size](void *region, size_t size,
                                                    int prot) {
    mprotect(region, round_to_next_page_size(size), prot);
  };

  Assembler assembler{mc};
  assembler.assemble();

  code_length = assembler.buffer.size();
  code = create_region(code_length);
  memcpy(code, assembler.buffer.data(), code_length);
  finalize_region(code, code_length, PROT_READ | PROT_EXEC);

  constants_length = mc.constants.size();
  constants = create_region(constants_length);
  memcpy(constants, mc.constants.data(), constants_length);
  finalize_region(constants, constants_length, PROT_READ);
}

void Executor::call(void *xs, void *ys, void *zs, void *results) {
  Executor::Function_Type *func =
      reinterpret_cast<Executor::Function_Type *>(code);
  func(xs, ys, zs, constants, results);
}

} // namespace sdfjit::machinecode
