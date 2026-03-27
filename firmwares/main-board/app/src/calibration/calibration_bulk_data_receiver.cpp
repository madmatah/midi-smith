#include "app/calibration/calibration_bulk_data_receiver.hpp"

#include "app/config/config.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"

namespace midismith::main_board::app::calibration {

CalibrationBulkDataReceiver::CalibrationBulkDataReceiver(
    midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
    CalibrationBulkDataReceiverObserverRequirements& observer,
    midismith::os::TimerRequirements& timeout_timer) noexcept
    : sender_(sender), observer_(observer), timeout_timer_(timeout_timer) {}

void CalibrationBulkDataReceiver::BeginReceiving(std::uint8_t board_id) noexcept {
  board_id_ = board_id;
  segment_transfer_tracker_.Reset();
  assembled_data_ = {};
  timeout_timer_.Start(midismith::main_board::app::config::kCalibrationReceiveTimeoutMs);
}

void CalibrationBulkDataReceiver::OnCalibrationDataSegment(
    const midismith::protocol::CalibrationDataSegment& segment,
    std::uint8_t source_node_id) noexcept {
  if (source_node_id != board_id_) {
    return;
  }

  UnpackSegmentIntoAssembledData(segment);

  sender_.SendCalibrationAck(board_id_, segment.seq_index,
                             midismith::protocol::DataSegmentAckStatus::kOk);

  segment_transfer_tracker_.OnSegmentReceived(segment.seq_index, segment.total_packets);

  if (segment_transfer_tracker_.IsComplete()) {
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
  midismith::protocol_can::CanCalibrationSegmentPacker::UnpackSegment(
      segment.payload.data(), segment.seq_index, assembled_data_.data(), assembled_data_.size());
}

}  // namespace midismith::main_board::app::calibration
