#include "app/supervisor/network_supervisor_task.hpp"

namespace midismith::main_board::app::supervisor {

void NetworkSupervisorTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, os::kWaitForever)) {
    std::visit(
        [this](auto&& event_variant) {
          using T = std::decay_t<decltype(event_variant)>;
          if constexpr (std::is_same_v<T, HeartbeatTick>) {
            sender_.SendHeartbeat(midismith::protocol::DeviceState::kRunning);
          } else if constexpr (std::is_same_v<T, HeartbeatReceived>) {
            peer_registry_.OnHeartbeatReceived(event_variant.node_id, event_variant.device_state,
                                               clock_.GetUptimeMs());
          } else if constexpr (std::is_same_v<T, TimeoutCheckTick>) {
            peer_registry_.CheckTimeout(clock_.GetUptimeMs());
          }
        },
        event);
  }
}

}  // namespace midismith::main_board::app::supervisor
