#pragma once

#include <array>
#include <cstdint>
#include <variant>

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/config/config.hpp"
#include "app/config/sensors.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationTask {
 public:
  using CalibrationArray =
      midismith::adc_board::app::analog::AcquisitionControlRequirements::CalibrationArray;
  using SegmentPayload =
      std::array<std::uint8_t, midismith::protocol::CalibrationDataSegment::kPayloadSizeBytes>;

  struct StartTransfer {
    CalibrationArray calibration_data;
  };
  struct AckReceived {
    midismith::protocol::DataSegmentAck ack;
  };
  struct TimeoutTick {};

  using Event = std::variant<StartTransfer, AckReceived, TimeoutTick>;

  static constexpr std::size_t kSensorsPerSegment =
      midismith::protocol::CalibrationDataSegment::kSensorsPerSegment;
  static constexpr std::size_t kTotalSegments =
      (midismith::adc_board::app::config::sensors::kSensorCount + kSensorsPerSegment - 1) /
      kSensorsPerSegment;
  static constexpr std::uint8_t kMaxRetries =
      static_cast<std::uint8_t>(midismith::adc_board::app::config::kCalibrationMaxRetries);

  CalibrationTask(midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
                  midismith::os::QueueRequirements<Event>& event_queue,
                  midismith::os::TimerRequirements& ack_timer) noexcept;

  void Run() noexcept;

  static void OnAckTimeout(void* ctx) noexcept;

 private:
  void HandleStartTransfer(const StartTransfer& event) noexcept;
  void HandleAckReceived(const AckReceived& event) noexcept;
  void HandleTimeoutTick() noexcept;
  void SendCurrentSegment() noexcept;
  SegmentPayload PackSegmentPayload(std::size_t segment_index) const noexcept;

  midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender_;
  midismith::os::QueueRequirements<Event>& event_queue_;
  midismith::os::TimerRequirements& ack_timer_;

  CalibrationArray calibration_data_{};
  std::size_t current_segment_index_ = 0;
  std::uint8_t retry_count_ = 0;
};

}  // namespace midismith::adc_board::app::calibration
