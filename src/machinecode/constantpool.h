#pragma once

#include <iostream>
#include <stdint.h>
#include <unordered_map>
#include <vector>

namespace sdfjit::machinecode {

// throw your constants in here, and get an offset into the pool that they'll be
// stored at (in little endian)
struct Constant_Pool {
  std::vector<uint8_t> memory{};
  static constexpr uint8_t pad_value = 0xcc;

  // caches:
  std::unordered_map<uint32_t, size_t> dword_cache{};

  // note that these are both unstable after a modifying operation
  uint8_t *data() { return memory.data(); }
  size_t size() const { return memory.size(); }

  template <size_t alignment> void align_to() {
    memory.reserve(memory.size() + alignment);
    while (memory.size() % alignment != 0) {
      memory.push_back(pad_value);
    }
  }

  void align_to_xmm() { align_to<128 / 8>(); }
  void align_to_ymm() { align_to<256 / 8>(); }

  size_t add(uint8_t byte) {
    auto offset = size();
    memory.push_back(byte);
    return offset;
  }

  size_t add(uint32_t dword) {
    auto cached = dword_cache.find(dword);
    if (cached != dword_cache.end()) {
      return cached->second;
    }

    auto offset = size();
    dword_cache[dword] = offset;
    for (size_t i = 0; i < sizeof(uint32_t); i++) {
      add(uint8_t(dword & 0xff));
      dword >>= 8;
    }
    return offset;
  }
};

std::ostream &operator<<(std::ostream &os, Constant_Pool &pool);

} // namespace sdfjit::machinecode
