#pragma once

#include "app/supervisor/network_supervisor_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::messaging {

class MainBoardInboundHeartbeatHandler final {
 public:
  explicit MainBoardInboundHeartbeatHandler(
      midismith::os::QueueRequirements<
          midismith::main_board::app::supervisor::NetworkSupervisorTask::Event>&
          event_queue) noexcept
      : event_queue_(event_queue) {}

  void OnHeartbeat(const midismith::protocol::Heartbeat& heartbeat,
                   std::uint8_t source_node_id) noexcept {
    using Task = midismith::main_board::app::supervisor::NetworkSupervisorTask;
    event_queue_.Send(Task::Event{Task::HeartbeatReceived{source_node_id, heartbeat.device_state}},
                      midismith::os::kNoWait);
  }

 private:
  midismith::os::QueueRequirements<
      midismith::main_board::app::supervisor::NetworkSupervisorTask::Event>& event_queue_;
};

}  // namespace midismith::main_board::app::messaging
