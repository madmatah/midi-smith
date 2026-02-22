#pragma once

#include <cstdint>

#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"

namespace midismith::adc_board::app::telemetry {

constexpr std::uint32_t DefaultSensorRttTelemetryOutputHz() noexcept {
  return (::midismith::adc_board::app::config::RTT_TELEMETRY_FREQUENCY_HZ <
          ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ)
             ? ::midismith::adc_board::app::config::RTT_TELEMETRY_FREQUENCY_HZ
             : ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
}

}  // namespace midismith::adc_board::app::telemetry
