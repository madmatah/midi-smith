#pragma once

#include <cstdint>

namespace bsp::adc {

struct AdcTriggerScheduleConfig {
  std::uint32_t channel_rate_hz = 1000;
  std::uint32_t adc2_phase_us = 0;
  std::uint32_t adc3_phase_us = 0;
};

class AdcTriggerSchedule final {
 public:
  bool Start(const AdcTriggerScheduleConfig& config) noexcept;
  void Stop() noexcept;

 private:
  bool running_ = false;
};

}  // namespace bsp::adc
