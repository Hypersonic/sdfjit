#include "perf_map_writer.h"

#include <fstream>
#include <unistd.h>

namespace sdfjit::profiling {

void add_perf_map_region(machinecode::Executor &exec, std::string_view name) {
  std::fstream out("/tmp/perf-" + std::to_string(getpid()) + ".map",
                   std::ios_base::app | std::ios_base::out);
  out << std::hex << exec.code << ' ' << exec.code_length << ' ' << name
      << std::endl
      << std::dec;
  out.close();
}

} // namespace sdfjit::profiling
