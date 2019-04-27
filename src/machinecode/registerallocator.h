#pragma once

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
};

struct Linear_Scan_Register_Allocator {
  void allocate(Machine_Code &mc);
  void expire_old_intervals(size_t i);
  void spill_at_interval(size_t i);

  void compute_live_intervals(Machine_Code &mc);

  Live_Interval_List live_intervals{};
};

} // namespace sdfjit::machinecode
