#pragma once

#include <cstdint>

#include "app/calibration/calibration_bulk_data_receiver.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class CalibrationCoordinatorInboundTarget {
 public:
  virtual ~CalibrationCoordinatorInboundTarget() = default;
  virtual void OnSensorEvent(std::uint8_t board_id, std::uint8_t sensor_id) noexcept = 0;
};

class MainBoardInboundCalibrationHandler {
 public:
  void SetReceiver(calibration::CalibrationBulkDataReceiver& receiver) noexcept {
    receiver_ = &receiver;
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

 private:
  calibration::CalibrationBulkDataReceiver* receiver_ = nullptr;
  CalibrationCoordinatorInboundTarget* coordinator_ = nullptr;
};

}  // namespace midismith::main_board::app::messaging
