#pragma once

#include "app/calibration/calibration_data_receiver.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcInboundCalibrationDataHandler final {
 public:
  void SetReceiver(
      midismith::adc_board::app::calibration::CalibrationDataReceiver& receiver) noexcept {
    receiver_ = &receiver;
  }

  void OnCalibrationDataSegment(const midismith::protocol::CalibrationDataSegment& segment,
                                std::uint8_t source_node_id) noexcept {
    if (receiver_ != nullptr) {
      receiver_->OnCalibrationDataSegment(segment, source_node_id);
    }
  }

 private:
  midismith::adc_board::app::calibration::CalibrationDataReceiver* receiver_ = nullptr;
};

}  // namespace midismith::adc_board::app::messaging
