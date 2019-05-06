#include "constantpool.h"

#include "util/hexdump.h"

namespace sdfjit::machinecode {

std::ostream &operator<<(std::ostream &os, Constant_Pool &pool) {
  util::hexdump(os, pool);
  return os;
}

} // namespace sdfjit::machinecode
