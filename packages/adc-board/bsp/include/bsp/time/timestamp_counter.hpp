#pragma once

#include <cstdint>

#include "app/time/timestamp_counter_requirements.hpp"

namespace bsp::time {

class TimestampCounter final : public app::time::TimestampCounterRequirements {
 public:
  using StartFn = void (*)() noexcept;
  using NowFn = std::uint32_t (*)() noexcept;

  TimestampCounter(StartFn start_fn, NowFn now_fn) noexcept
      : start_fn_(start_fn), now_fn_(now_fn) {}

  void Start() noexcept override {
    if (start_fn_ != nullptr) {
      start_fn_();
    }
  }

  std::uint32_t NowTicks() const noexcept override {
    if (now_fn_ == nullptr) {
      return 0;
    }
    return now_fn_();
  }

 private:
  StartFn start_fn_ = nullptr;
  NowFn now_fn_ = nullptr;
};

}  // namespace bsp::time
