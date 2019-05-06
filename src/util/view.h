#pragma once

#include <vector>

namespace sdfjit::util {

// provides a view into a vector of Ts, with some offset into that collection
// and a length
//
// Like string_view and co, does not own the backing vector
template <typename T> struct vector_view {
  const std::vector<T> &backing;
  size_t offset{0};
  size_t length{backing.size()};

  const T *data() const { return backing.data() + offset; }
  size_t size() const { return length; }
};

} // namespace sdfjit::util
