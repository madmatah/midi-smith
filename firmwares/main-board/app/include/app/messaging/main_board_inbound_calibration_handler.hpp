#pragma once

#include <cstdint>

#include "app/calibration/calibration_bulk_data_receiver.hpp"
#include "app/calibration/calibration_data_server_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class CalibrationCoordinatorInboundTarget {
 public:
  virtual ~CalibrationCoordinatorInboundTarget() = default;
  virtual void OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept = 0;
};

class MainBoardInboundCalibrationHandler {
 public:
  using ServerEvent = midismith::main_board::app::calibration::CalibrationDataServerTask::Event;

  void SetReceiver(calibration::CalibrationBulkDataReceiver& receiver) noexcept {
    receiver_ = &receiver;
  }

  void SetServerEventQueue(
      midismith::os::QueueRequirements<ServerEvent>& server_event_queue) noexcept {
    server_event_queue_ = &server_event_queue;
  }

  void SetCoordinator(CalibrationCoordinatorInboundTarget& coordinator) noexcept {
    coordinator_ = &coordinator;
  }

  void OnCalibrationDataSegment(const protocol::CalibrationDataSegment& segment,
                                std::uint8_t source_node_id) noexcept {
    if (receiver_ != nullptr) {
      receiver_->OnCalibrationDataSegment(segment, source_node_id);
    }
  }

  void OnSensorEvent(const protocol::SensorEvent& event, std::uint8_t source_node_id) noexcept {
    if (coordinator_ != nullptr) {
      coordinator_->OnSensorEvent(source_node_id, event.sensor_id);
    }
  }

  void OnCalibrationLoadRequest(const protocol::CalibrationLoadRequest&,
                                std::uint8_t source_node_id) noexcept {
    if (server_event_queue_ != nullptr) {
      server_event_queue_->Send(
          ServerEvent{
              midismith::main_board::app::calibration::CalibrationDataServerTask::LoadRequest{
                  source_node_id}},
          midismith::os::kNoWait);
    }
  }

  void OnDataSegmentAck(const protocol::DataSegmentAck& ack, std::uint8_t source_node_id) noexcept {
    if (server_event_queue_ != nullptr) {
      server_event_queue_->Send(
          ServerEvent{
              midismith::main_board::app::calibration::CalibrationDataServerTask::AckReceived{
                  .source_node_id = source_node_id, .ack = ack}},
          midismith::os::kNoWait);
    }
  }

 private:
  calibration::CalibrationBulkDataReceiver* receiver_ = nullptr;
  CalibrationCoordinatorInboundTarget* coordinator_ = nullptr;
  midismith::os::QueueRequirements<ServerEvent>* server_event_queue_ = nullptr;
};

}  // namespace midismith::main_board::app::messaging
