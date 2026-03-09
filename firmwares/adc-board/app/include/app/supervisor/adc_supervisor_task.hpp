#pragma once

#include <cstdint>
#include <variant>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_monitor.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"

namespace midismith::adc_board::app::supervisor {

class AdcSupervisorTask {
 public:
  struct HeartbeatTick {};
  struct HeartbeatReceived {
    midismith::protocol::DeviceState device_state;
  };
  struct TimeoutCheckTick {};
  struct InitializationComplete {};
  using Event =
      std::variant<HeartbeatTick, HeartbeatReceived, TimeoutCheckTick, InitializationComplete>;

  AdcSupervisorTask(messaging::AdcBoardMessageSenderRequirements& sender,
                    analog::AcquisitionStateRequirements& acquisition_state,
                    midismith::os::QueueRequirements<Event>& event_queue,
                    midismith::protocol::PeerMonitorObserverRequirements& peer_state_observer,
                    midismith::os::UptimeProviderRequirements& clock,
                    std::uint32_t peer_timeout_ms) noexcept
      : sender_(sender),
        acquisition_state_(acquisition_state),
        event_queue_(event_queue),
        peer_monitor_(peer_timeout_ms, peer_state_observer),
        clock_(clock) {}

  void Run() noexcept;

 private:
  midismith::protocol::DeviceState CurrentDeviceState() const noexcept;

  messaging::AdcBoardMessageSenderRequirements& sender_;
  analog::AcquisitionStateRequirements& acquisition_state_;
  midismith::os::QueueRequirements<Event>& event_queue_;
  midismith::protocol::PeerMonitor peer_monitor_;
  midismith::os::UptimeProviderRequirements& clock_;
  bool initialization_complete_{false};
};

}  // namespace midismith::adc_board::app::supervisor
