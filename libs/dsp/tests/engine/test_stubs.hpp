#pragma once

#include <cstdint>

namespace midismith::dsp::engine::test {

struct TestContext {
  std::uint32_t consume_count = 0;
  float last_value = 0.0f;
};

class CounterStage {
 public:
  void Reset() noexcept {
    compute_count = 0;
    process_count = 0;
    push_count = 0;
    reset_count++;
  }

  void Push(float sample, const TestContext& ctx) noexcept {
    (void) sample;
    (void) ctx;
    push_count++;
  }

  float Compute(float sample, const TestContext& ctx) const noexcept {
    (void) ctx;
    compute_count++;
    return sample + 1.0f;
  }

  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    process_count++;
    return sample + 1.0f;
  }

  static void ResetCounts() noexcept {
    compute_count = 0;
    process_count = 0;
    push_count = 0;
    reset_count = 0;
  }

  static inline std::uint32_t compute_count = 0;
  static inline std::uint32_t process_count = 0;
  static inline std::uint32_t push_count = 0;
  static inline std::uint32_t reset_count = 0;
};

class PlusTenStage {
 public:
  void Reset() noexcept {}

  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    return sample + 10.0f;
  }
};

class TimesTwoStage {
 public:
  void Reset() noexcept {}

  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    return sample * 2.0f;
  }
};

class ConsumerStage {
 public:
  void Reset() noexcept {}

  void Execute(float sample, TestContext& ctx) noexcept {
    ctx.consume_count++;
    ctx.last_value = sample;
  }
};

}  // namespace midismith::dsp::engine::test
