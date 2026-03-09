#include "app/supervisor/network_supervisor_task.hpp"

namespace midismith::main_board::app::supervisor {

void NetworkSupervisorTask::Run() noexcept {
  if constexpr (config::kAutoStartPowerSequenceOnBoot) {
    boards_controller_.StartPowerSequence();
  }

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
            boards_controller_.CheckSequenceTimeout();
          } else if constexpr (std::is_same_v<T, PowerOnCommand>) {
            boards_controller_.PowerOn(event_variant.peer_id);
          } else if constexpr (std::is_same_v<T, PowerOffCommand>) {
            boards_controller_.PowerOff(event_variant.peer_id);
          } else if constexpr (std::is_same_v<T, StartPowerSequenceCommand>) {
            boards_controller_.StartPowerSequence();
          } else if constexpr (std::is_same_v<T, StopAllCommand>) {
            boards_controller_.StopAll();
          }
        },
        event);
  }
}

}  // namespace midismith::main_board::app::supervisor
