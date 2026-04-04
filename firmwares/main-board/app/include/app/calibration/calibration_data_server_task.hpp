#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>

#include "app/config/config.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "app/storage/calibration_persistent_store.hpp"
#include "calibration/sensor_calibration.hpp"
#include "domain/calibration/calibration_data.hpp"
#include "domain/config/main_board_config.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::calibration {

class CalibrationDataServerTask {
 public:
  using SensorCalibrationArray =
      std::array<midismith::calibration::SensorCalibration,
                 midismith::main_board::domain::config::kSensorsPerBoard>;
  using SegmentPayload =
      std::array<std::uint8_t,
                 midismith::protocol_can::CanCalibrationSegmentPacker::kSegmentPayloadSizeBytes>;

  struct LoadRequest {
    std::uint8_t board_id;
  };

  struct AckReceived {
    std::uint8_t source_node_id;
    midismith::protocol::DataSegmentAck ack;
  };

  struct AckTimeout {};

  using Event = std::variant<LoadRequest, AckReceived, AckTimeout>;

  static constexpr std::size_t kTotalSegments =
      midismith::protocol_can::CanCalibrationSegmentPacker::ComputeTotalSegments(
          midismith::main_board::domain::config::kSensorsPerBoard);
  static constexpr std::uint8_t kMaxRetries =
      static_cast<std::uint8_t>(midismith::main_board::app::config::kCalibrationServerMaxRetries);

  CalibrationDataServerTask(
      midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender,
      midismith::main_board::app::storage::CalibrationPersistentStore& store,
      midismith::os::QueueRequirements<Event>& event_queue,
      midismith::os::TimerRequirements& ack_timer) noexcept;

  void Run() noexcept;

  static void OnAckTimeout(void* ctx) noexcept;

 private:
  static constexpr std::size_t kMaxPendingRequests =
      midismith::main_board::domain::config::kMaxBoardCount;
  static constexpr std::uint8_t kMaxBoardId = midismith::main_board::domain::config::kMaxBoardCount;

  void HandleLoadRequest(const LoadRequest& event) noexcept;
  void HandleAckReceived(const AckReceived& event) noexcept;
  void HandleAckTimeout() noexcept;
  void StartTransferForBoard(std::uint8_t board_id) noexcept;
  void SendCurrentSegment() noexcept;
  SegmentPayload PackSegmentPayload(std::size_t segment_index) const noexcept;
  void EnqueuePendingRequest(std::uint8_t board_id) noexcept;
  void StartNextPendingRequest() noexcept;
  bool HasValidBoardId(std::uint8_t board_id) const noexcept;

  midismith::main_board::app::messaging::MainBoardMessageSenderRequirements& sender_;
  midismith::main_board::app::storage::CalibrationPersistentStore& store_;
  midismith::os::QueueRequirements<Event>& event_queue_;
  midismith::os::TimerRequirements& ack_timer_;

  midismith::main_board::domain::calibration::CalibrationData stored_calibration_data_{};
  SensorCalibrationArray calibration_data_{};
  std::array<std::uint8_t, kMaxPendingRequests> pending_board_ids_{};
  std::size_t pending_request_head_index_ = 0;
  std::size_t pending_request_count_ = 0;
  std::uint8_t active_board_id_ = 0;
  std::size_t current_segment_index_ = 0;
  std::uint8_t retry_count_ = 0;
  bool transfer_active_ = false;
};

}  // namespace midismith::main_board::app::calibration
