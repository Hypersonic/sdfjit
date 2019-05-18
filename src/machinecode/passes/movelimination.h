#pragma once

#include "machinecode/machinecode.h"

namespace sdfjit::machinecode::passes {

void peephole_eliminate_movs(Machine_Code &mc);

}
