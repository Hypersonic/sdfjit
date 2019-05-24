#pragma once

#include <chrono>

struct Frame_Counter {
  using Time_Point = std::chrono::time_point<std::chrono::steady_clock>;

  Time_Point begin{std::chrono::steady_clock::now()};
  Time_Point end{};
  size_t frames;

  void stop() { end = std::chrono::steady_clock::now(); }
  void tick_frame() { frames++; }

  float fps() const {
    return float(frames) / std::chrono::duration<float>(end - begin).count();
  }
};
