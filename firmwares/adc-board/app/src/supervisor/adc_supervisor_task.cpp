#include "app/supervisor/adc_supervisor_task.hpp"

#include "app/analog/acquisition_state.hpp"

namespace midismith::adc_board::app::supervisor {

void AdcSupervisorTask::Run() noexcept {
  Event event;
  while (event_queue_.Receive(event, os::kWaitForever)) {
    std::visit([this](const HeartbeatTick&) { sender_.SendHeartbeat(CurrentDeviceState()); },
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
