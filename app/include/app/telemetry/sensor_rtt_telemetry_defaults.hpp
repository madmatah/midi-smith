#pragma once

#include <cstdint>

#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"

namespace app::telemetry {

constexpr std::uint32_t DefaultSensorRttTelemetryOutputHz() noexcept {
  return (::app::config::RTT_TELEMETRY_FREQUENCY_HZ <
          ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ)
             ? ::app::config::RTT_TELEMETRY_FREQUENCY_HZ
             : ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
}

}  // namespace app::telemetry
