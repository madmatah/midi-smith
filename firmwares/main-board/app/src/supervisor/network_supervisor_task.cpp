#include "app/supervisor/network_supervisor_task.hpp"


namespace midismith::main_board::app::supervisor {

void NetworkSupervisorTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, os::kWaitForever)) {
    std::visit(
        [this](const HeartbeatTick&) { sender_.SendHeartbeat(protocol::DeviceState::kRunning); },
        event);
  }
}

}  // namespace midismith::main_board::app::supervisor
