#pragma once

#include <string_view>

#include "machinecode/executor.h"

namespace sdfjit::profiling {

void add_perf_map_region(machinecode::Executor &exec, std::string_view name);

}
