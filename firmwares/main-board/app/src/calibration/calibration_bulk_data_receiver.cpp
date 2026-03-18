#include "app/calibration/calibration_bulk_data_receiver.hpp"

#include <cstring>

#include "app/config/config.hpp"

namespace midismith::main_board::app::calibration {

CalibrationBulkDataReceiver::CalibrationBulkDataReceiver(
    midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
    CalibrationBulkDataReceiverObserverRequirements& observer,
    midismith::os::TimerRequirements& timeout_timer) noexcept
    : sender_(sender), observer_(observer), timeout_timer_(timeout_timer) {}

void CalibrationBulkDataReceiver::BeginReceiving(std::uint8_t board_id) noexcept {
  board_id_ = board_id;
  expected_total_segments_ = 0;
  received_segments_bitmask_ = 0;
  assembled_data_ = {};
  timeout_timer_.Start(midismith::main_board::app::config::kCalibrationReceiveTimeoutMs);
}

void CalibrationBulkDataReceiver::OnCalibrationDataSegment(
    const midismith::protocol::CalibrationDataSegment& segment,
    std::uint8_t source_node_id) noexcept {
  if (source_node_id != board_id_) {
    return;
  }

  expected_total_segments_ = segment.total_packets;
  UnpackSegmentIntoAssembledData(segment);

  sender_.SendCalibrationAck(board_id_, segment.seq_index,
                             midismith::protocol::DataSegmentAckStatus::kOk);

  received_segments_bitmask_ |= static_cast<std::uint8_t>(1u << segment.seq_index);

  if (AllSegmentsReceived()) {
    timeout_timer_.Stop();
    observer_.OnDataReceived(board_id_, assembled_data_);
  }
}

void CalibrationBulkDataReceiver::OnReceiveTimeout(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<CalibrationBulkDataReceiver*>(ctx)->HandleTimeout();
  }
}

void CalibrationBulkDataReceiver::HandleTimeout() noexcept {
  observer_.OnReceiveTimeout(board_id_);
}

void CalibrationBulkDataReceiver::UnpackSegmentIntoAssembledData(
    const midismith::protocol::CalibrationDataSegment& segment) noexcept {
  for (std::size_t slot = 0; slot < kSensorsPerSegment; ++slot) {
    const std::size_t sensor_index = segment.seq_index * kSensorsPerSegment + slot;
    if (sensor_index < kSensorsPerBoard) {
      std::memcpy(&assembled_data_[sensor_index],
                  segment.payload.data() + slot * kSensorCalibrationSizeBytes,
                  sizeof(midismith::sensor_linearization::SensorCalibration));
    }
  }
}

bool CalibrationBulkDataReceiver::AllSegmentsReceived() const noexcept {
  if (expected_total_segments_ == 0) {
    return false;
  }
  const std::uint8_t full_mask = static_cast<std::uint8_t>((1u << expected_total_segments_) - 1u);
  return (received_segments_bitmask_ & full_mask) == full_mask;
}

}  // namespace midismith::main_board::app::calibration
