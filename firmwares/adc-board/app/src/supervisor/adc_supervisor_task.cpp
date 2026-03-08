#include "app/supervisor/adc_supervisor_task.hpp"

#include "app/analog/acquisition_state.hpp"

namespace midismith::adc_board::app::supervisor {

void AdcSupervisorTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, os::kWaitForever)) {
    std::visit(
        [this](auto&& event_variant) {
          using T = std::decay_t<decltype(event_variant)>;
          if constexpr (std::is_same_v<T, HeartbeatTick>) {
            sender_.SendHeartbeat(CurrentDeviceState());
          } else if constexpr (std::is_same_v<T, HeartbeatReceived>) {
            peer_monitor_.OnHeartbeatReceived(event_variant.device_state, clock_.GetUptimeMs());
          } else if constexpr (std::is_same_v<T, TimeoutCheckTick>) {
            peer_monitor_.CheckTimeout(clock_.GetUptimeMs());
          }
        },
        event);
  }
}

protocol::DeviceState AdcSupervisorTask::CurrentDeviceState() const noexcept {
  if (acquisition_state_.GetState() == analog::AcquisitionState::kEnabled) {
    return protocol::DeviceState::kRunning;
  }
  return protocol::DeviceState::kIdle;
}

}  // namespace midismith::adc_board::app::supervisor
