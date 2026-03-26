#pragma once

#include "app/calibration/calibration_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcInboundAckHandler final {
 public:
  explicit AdcInboundAckHandler(
      midismith::os::QueueRequirements<
          midismith::adc_board::app::calibration::CalibrationTask::Event>& event_queue) noexcept
      : event_queue_(event_queue) {}

  void OnDataSegmentAck(const midismith::protocol::DataSegmentAck& ack,
                        std::uint8_t /*source_node_id*/) noexcept {
    using AckReceived = midismith::adc_board::app::calibration::CalibrationTask::AckReceived;
    event_queue_.Send(AckReceived{ack}, midismith::os::kNoWait);
  }

 private:
  midismith::os::QueueRequirements<midismith::adc_board::app::calibration::CalibrationTask::Event>&
      event_queue_;
};

}  // namespace midismith::adc_board::app::messaging
