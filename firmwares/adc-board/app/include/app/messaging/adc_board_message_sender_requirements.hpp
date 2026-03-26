#pragma once

#include <array>
#include <cstdint>

#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcBoardMessageSenderRequirements {
 public:
  virtual ~AdcBoardMessageSenderRequirements() = default;

  virtual bool SendNoteOn(std::uint8_t sensor_id, std::uint8_t velocity) noexcept = 0;
  virtual bool SendNoteOff(std::uint8_t sensor_id, std::uint8_t velocity) noexcept = 0;
  virtual bool SendHeartbeat(protocol::DeviceState device_state) noexcept = 0;
  virtual bool SendCalibrationDataSegment(
      std::uint8_t seq_index, std::uint8_t total_packets,
      const std::array<std::uint8_t, protocol::CalibrationDataSegment::kPayloadSizeBytes>&
          payload) noexcept = 0;
};

}  // namespace midismith::adc_board::app::messaging
