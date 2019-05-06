#pragma once

#include <iomanip>
#include <iostream>

namespace sdfjit::util {

template <typename T> void hexdump(std::ostream &os, T &vec) {
  static constexpr size_t num_cols = 8;

  os << std::hex;
  auto old_fill = os.fill();
  auto old_width = os.width();

  for (size_t i = 0; i < vec.size(); i++) {
    if (i % num_cols == 0) {
      os << std::setfill(' ') << std::setw(4) << i << ": ";
    }

    os << std::setfill('0') << std::setw(2) << uint32_t(vec.data()[i]);

    if (i % num_cols == num_cols - 1) {
      os << std::endl;
    } else {
      os << ' ';
    }
  }

  os.fill(old_fill);
  os.width(old_width);
  os << std::endl << std::dec;
}

} // namespace sdfjit::util
