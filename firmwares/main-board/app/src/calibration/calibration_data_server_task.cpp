#include "app/calibration/calibration_data_server_task.hpp"

#include <type_traits>

#include "os-types/queue_requirements.hpp"

namespace midismith::main_board::app::calibration {

CalibrationDataServerTask::CalibrationDataServerTask(
    midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
    midismith::main_board::app::storage::CalibrationPersistentStore& store,
    midismith::os::QueueRequirements<Event>& event_queue,
    midismith::os::TimerRequirements& ack_timer) noexcept
    : sender_(sender), store_(store), event_queue_(event_queue), ack_timer_(ack_timer) {}

void CalibrationDataServerTask::OnAckTimeout(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }

  static_cast<midismith::os::QueueRequirements<Event>*>(ctx)->Send(AckTimeout{},
                                                                   midismith::os::kNoWait);
}

void CalibrationDataServerTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, midismith::os::kWaitForever)) {
    std::visit(
        [this](const auto& typed_event) noexcept {
          using T = std::decay_t<decltype(typed_event)>;
          if constexpr (std::is_same_v<T, LoadRequest>) {
            HandleLoadRequest(typed_event);
          } else if constexpr (std::is_same_v<T, AckReceived>) {
            HandleAckReceived(typed_event);
          } else if constexpr (std::is_same_v<T, AckTimeout>) {
            HandleAckTimeout();
          }
        },
        event);
  }
}

void CalibrationDataServerTask::HandleLoadRequest(const LoadRequest& event) noexcept {
  if (!HasValidBoardId(event.board_id)) {
    return;
  }

  if (transfer_active_) {
    EnqueuePendingRequest(event.board_id);
    return;
  }

  StartTransferForBoard(event.board_id);
}

void CalibrationDataServerTask::HandleAckReceived(const AckReceived& event) noexcept {
  if (!transfer_active_) {
    return;
  }

  if (event.source_node_id != active_board_id_) {
    return;
  }

  if (event.ack.ack_index != current_segment_index_) {
    return;
  }

  ack_timer_.Stop();

  if (event.ack.status != midismith::protocol::DataSegmentAckStatus::kOk) {
    ++retry_count_;
    if (retry_count_ >= kMaxRetries) {
      transfer_active_ = false;
      StartNextPendingRequest();
      return;
    }
    SendCurrentSegment();
    return;
  }

  retry_count_ = 0;
  ++current_segment_index_;
  if (current_segment_index_ >= kTotalSegments) {
    transfer_active_ = false;
    StartNextPendingRequest();
    return;
  }

  SendCurrentSegment();
}

void CalibrationDataServerTask::HandleAckTimeout() noexcept {
  if (!transfer_active_) {
    return;
  }

  ++retry_count_;
  if (retry_count_ >= kMaxRetries) {
    transfer_active_ = false;
    StartNextPendingRequest();
    return;
  }

  SendCurrentSegment();
}

void CalibrationDataServerTask::StartTransferForBoard(std::uint8_t board_id) noexcept {
  midismith::main_board::domain::calibration::CalibrationData stored_calibration_data{};
  store_.Load(stored_calibration_data);

  const std::size_t board_index = static_cast<std::size_t>(board_id - 1u);
  if (!stored_calibration_data.board_data_valid[board_index]) {
    sender_.SendCalibrationDataSegment(board_id, 0, 0, SegmentPayload{});
    StartNextPendingRequest();
    return;
  }

  calibration_data_ = stored_calibration_data.sensor_calibrations[board_index];
  active_board_id_ = board_id;
  current_segment_index_ = 0;
  retry_count_ = 0;
  transfer_active_ = true;
  SendCurrentSegment();
}

void CalibrationDataServerTask::SendCurrentSegment() noexcept {
  const auto payload = PackSegmentPayload(current_segment_index_);
  sender_.SendCalibrationDataSegment(active_board_id_,
                                     static_cast<std::uint8_t>(current_segment_index_),
                                     static_cast<std::uint8_t>(kTotalSegments), payload);
  ack_timer_.Start(midismith::main_board::app::config::kCalibrationServerAckTimeoutMs);
}

CalibrationDataServerTask::SegmentPayload CalibrationDataServerTask::PackSegmentPayload(
    std::size_t segment_index) const noexcept {
  SegmentPayload payload{};
  midismith::protocol_can::CanCalibrationSegmentPacker::PackSegment(
      calibration_data_.data(), calibration_data_.size(), segment_index, payload.data());
  return payload;
}

void CalibrationDataServerTask::EnqueuePendingRequest(std::uint8_t board_id) noexcept {
  if (pending_request_count_ >= kMaxPendingRequests) {
    return;
  }

  const std::size_t write_index =
      (pending_request_head_index_ + pending_request_count_) % kMaxPendingRequests;
  pending_board_ids_[write_index] = board_id;
  ++pending_request_count_;
}

void CalibrationDataServerTask::StartNextPendingRequest() noexcept {
  while (!transfer_active_ && pending_request_count_ > 0) {
    const std::uint8_t next_board_id = pending_board_ids_[pending_request_head_index_];
    pending_request_head_index_ = (pending_request_head_index_ + 1u) % kMaxPendingRequests;
    --pending_request_count_;
    StartTransferForBoard(next_board_id);
  }
}

bool CalibrationDataServerTask::HasValidBoardId(std::uint8_t board_id) const noexcept {
  return board_id >= 1u && board_id <= kMaxBoardId;
}

}  // namespace midismith::main_board::app::calibration
