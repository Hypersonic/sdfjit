#pragma once

#include <algorithm>
#include <unordered_map>

#include "machinecode.h"

namespace sdfjit::machinecode {

struct Live_Interval {
  Register reg;
  // these are both _inclusive_
  // the first counts for the outparams of the first instruction that uses reg
  // the last counts for the inparams of the last instruction that uses reg
  size_t first{0};
  size_t last{0};
};

struct Live_Interval_List {
  std::vector<Live_Interval> intervals{};

  Live_Interval &at(const Register &reg) {
    for (Live_Interval &interval : intervals) {
      if (interval.reg == reg) {
        return interval;
      }
    }
    abort();
  }

  Live_Interval &operator[](const Register &reg) {
    for (Live_Interval &interval : intervals) {
      if (interval.reg == reg) {
        return interval;
      }
    }
    intervals.push_back(Live_Interval{reg});
    return intervals.back();
  }

  bool contains(const Register &reg) {
    for (Live_Interval &interval : intervals) {
      if (interval.reg == reg) {
        return true;
      }
    }
    return false;
  }

  std::vector<Register> registers_live_at(size_t time) {
    std::vector<Register> result{};

    for (auto &interval : intervals) {
      if (interval.first <= time && time <= interval.last) {
        result.push_back(interval.reg);
      }
    }

    return result;
  }

  Live_Interval_List sorted_by_start_point() const {
    Live_Interval_List result{intervals};
    std::sort(
        result.intervals.begin(), result.intervals.end(),
        [](Live_Interval &a, Live_Interval &b) { return a.first < b.first; });
    return result;
  }

  std::vector<Live_Interval>::iterator begin() { return intervals.begin(); }
  std::vector<Live_Interval>::iterator end() { return intervals.end(); }
  size_t size() const { return intervals.size(); }
};

struct Linear_Scan_Register_Allocator {
  void allocate(Machine_Code &mc);
  void compute_live_intervals(Machine_Code &mc);

  Live_Interval_List live_intervals{};

  // registers we can use for anything
  std::vector<Machine_Register> machine_registers{
      Machine_Register::ymm0, Machine_Register::ymm1, Machine_Register::ymm2,
      Machine_Register::ymm3, Machine_Register::ymm4, Machine_Register::ymm5,
  };
  // reserved register for holding spilled values while being worked on
  // we need to reserve at least as many registers as the largest number of
  // parameters an instruction can take so we can load them all if needed
  std::vector<Machine_Register> temp_regs{
      Machine_Register::ymm6,
      Machine_Register::ymm7,
  };
};

} // namespace sdfjit::machinecode
