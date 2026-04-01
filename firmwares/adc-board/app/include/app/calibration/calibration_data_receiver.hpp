#pragma once

#include <cstdint>

#include "app/calibration/calibration_data_receiver_observer_requirements.hpp"
#include "app/config/sensors.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "calibration/board_calibration_data.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/transfer/segment_transfer_tracker.hpp"

namespace midismith::adc_board::app::calibration {

class CalibrationDataReceiver {
 public:
  using SensorCalibrationArray =
      CalibrationDataReceiverObserverRequirements::SensorCalibrationArray;

  CalibrationDataReceiver(
      midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
      CalibrationDataReceiverObserverRequirements& observer,
      midismith::os::TimerRequirements& timeout_timer) noexcept;

  void BeginReceiving() noexcept;

  void OnCalibrationDataSegment(const midismith::protocol::CalibrationDataSegment& segment,
                                std::uint8_t source_node_id) noexcept;

  static void OnReceiveTimeout(void* ctx) noexcept;

 private:
  void HandleTimeout() noexcept;
  void UnpackSegmentIntoAssembledData(
      const midismith::protocol::CalibrationDataSegment& segment) noexcept;

  midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender_;
  CalibrationDataReceiverObserverRequirements& observer_;
  midismith::os::TimerRequirements& timeout_timer_;

  midismith::protocol::transfer::SegmentTransferTracker segment_transfer_tracker_{};
  SensorCalibrationArray assembled_data_{};
};

}  // namespace midismith::adc_board::app::calibration
