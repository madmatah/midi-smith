#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::adc_board::app::telemetry {

std::size_t BuildSensorRttSchemaFrame(std::uint8_t sensor_id, std::uint32_t timestamp_us,
                                      std::span<std::uint8_t> out) noexcept;

}  // namespace midismith::adc_board::app::telemetry
