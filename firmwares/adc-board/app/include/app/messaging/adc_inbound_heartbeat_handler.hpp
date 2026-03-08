#pragma once

#include "app/supervisor/adc_supervisor_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcInboundHeartbeatHandler final {
 public:
  explicit AdcInboundHeartbeatHandler(
      midismith::os::QueueRequirements<
          midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>& event_queue) noexcept
      : event_queue_(event_queue) {}

  void OnHeartbeat(const midismith::protocol::Heartbeat& heartbeat,
                   std::uint8_t /*source_node_id*/) noexcept {
    using Task = midismith::adc_board::app::supervisor::AdcSupervisorTask;
    event_queue_.Send(Task::Event{Task::HeartbeatReceived{heartbeat.device_state}},
                      midismith::os::kNoWait);
  }

 private:
  midismith::os::QueueRequirements<midismith::adc_board::app::supervisor::AdcSupervisorTask::Event>&
      event_queue_;
};

}  // namespace midismith::adc_board::app::messaging
