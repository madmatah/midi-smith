#pragma once

#include <cstdint>
#include <variant>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_registry.hpp"
#include "protocol/peer_registry_observer_requirements.hpp"

namespace midismith::main_board::app::supervisor {

class NetworkSupervisorTask {
 public:
  struct HeartbeatTick {};
  struct HeartbeatReceived {
    std::uint8_t node_id;
    midismith::protocol::DeviceState device_state;
  };
  struct TimeoutCheckTick {};
  using Event = std::variant<HeartbeatTick, HeartbeatReceived, TimeoutCheckTick>;

  static constexpr std::size_t kMaxPeerCount{8};

  NetworkSupervisorTask(
      messaging::MainBoardMessageSenderRequirements& sender,
      midismith::os::QueueRequirements<Event>& event_queue,
      midismith::protocol::PeerRegistryObserverRequirements& peer_registry_observer,
      midismith::os::UptimeProviderRequirements& clock, std::uint32_t peer_timeout_ms) noexcept
      : sender_(sender),
        event_queue_(event_queue),
        peer_registry_(peer_timeout_ms, peer_registry_observer),
        clock_(clock) {}

  void Run() noexcept;

 private:
  messaging::MainBoardMessageSenderRequirements& sender_;
  midismith::os::QueueRequirements<Event>& event_queue_;
  midismith::protocol::PeerRegistry<kMaxPeerCount> peer_registry_;
  midismith::os::UptimeProviderRequirements& clock_;
};

}  // namespace midismith::main_board::app::supervisor
