#include "app/calibration/calibration_data_receiver.hpp"

#include "app/config/config.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"
#include "protocol/topology.hpp"

namespace midismith::adc_board::app::calibration {

CalibrationDataReceiver::CalibrationDataReceiver(
    midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements& sender,
    CalibrationDataReceiverObserverRequirements& observer,
    midismith::os::TimerRequirements& timeout_timer) noexcept
    : sender_(sender), observer_(observer), timeout_timer_(timeout_timer) {}

void CalibrationDataReceiver::BeginReceiving() noexcept {
  segment_transfer_tracker_.Reset();
  assembled_data_ = {};
  timeout_timer_.Start(config::kCalibrationLoadSegmentTimeoutMs);
}

void CalibrationDataReceiver::OnCalibrationDataSegment(
    const midismith::protocol::CalibrationDataSegment& segment,
    std::uint8_t source_node_id) noexcept {
  if (source_node_id != midismith::protocol::kMainBoardNodeId) {
    return;
  }

  if (segment.total_packets == 0) {
    timeout_timer_.Stop();
    observer_.OnCalibrationNoDataAvailable();
    return;
  }

  UnpackSegmentIntoAssembledData(segment);

  sender_.SendDataSegmentAck(segment.seq_index, midismith::protocol::DataSegmentAckStatus::kOk);

  segment_transfer_tracker_.OnSegmentReceived(segment.seq_index, segment.total_packets);

  timeout_timer_.Start(config::kCalibrationLoadSegmentTimeoutMs);

  if (segment_transfer_tracker_.IsComplete()) {
    timeout_timer_.Stop();
    observer_.OnCalibrationDataReceived(assembled_data_);
  }
}

void CalibrationDataReceiver::OnReceiveTimeout(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<CalibrationDataReceiver*>(ctx)->HandleTimeout();
  }
}

void CalibrationDataReceiver::HandleTimeout() noexcept {
  observer_.OnCalibrationReceiveTimeout();
}

void CalibrationDataReceiver::UnpackSegmentIntoAssembledData(
    const midismith::protocol::CalibrationDataSegment& segment) noexcept {
  midismith::protocol_can::CanCalibrationSegmentPacker::UnpackSegment(
      segment.payload.data(), segment.seq_index, assembled_data_.data(), assembled_data_.size());
}

}  // namespace midismith::adc_board::app::calibration
