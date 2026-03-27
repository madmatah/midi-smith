#pragma once

#include <array>
#include <cstdint>

#include "app/calibration/calibration_bulk_data_receiver_observer_requirements.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "calibration/board_calibration_data.hpp"
#include "domain/config/main_board_config.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/transfer/segment_transfer_tracker.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationBulkDataReceiver {
 public:
  using SensorCalibrationArray =
      CalibrationBulkDataReceiverObserverRequirements::SensorCalibrationArray;

  CalibrationBulkDataReceiver(
      midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
      CalibrationBulkDataReceiverObserverRequirements& observer,
      midismith::os::TimerRequirements& timeout_timer) noexcept;

  void BeginReceiving(std::uint8_t board_id) noexcept;

  void OnCalibrationDataSegment(const midismith::protocol::CalibrationDataSegment& segment,
                                std::uint8_t source_node_id) noexcept;

  static void OnReceiveTimeout(void* ctx) noexcept;

 private:
  static constexpr std::uint8_t kSensorsPerBoard =
      midismith::main_board::domain::config::kSensorsPerBoard;

  void HandleTimeout() noexcept;
  void UnpackSegmentIntoAssembledData(
      const midismith::protocol::CalibrationDataSegment& segment) noexcept;

  midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender_;
  CalibrationBulkDataReceiverObserverRequirements& observer_;
  midismith::os::TimerRequirements& timeout_timer_;

  std::uint8_t board_id_ = 0;
  midismith::protocol::transfer::SegmentTransferTracker segment_transfer_tracker_{};
  SensorCalibrationArray assembled_data_{};
};

}  // namespace midismith::main_board::app::calibration
