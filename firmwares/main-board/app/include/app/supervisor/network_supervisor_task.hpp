#pragma once

#include <cstdint>
#include <variant>

#include "app/adc/adc_boards_controller.hpp"
#include "app/config/config.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"
#include "protocol/messages.hpp"
#include "protocol/peer_registry.hpp"

namespace midismith::main_board::app::supervisor {

class NetworkSupervisorTask {
 public:
  struct HeartbeatTick {};
  struct HeartbeatReceived {
    std::uint8_t node_id;
    midismith::protocol::DeviceState device_state;
  };
  struct TimeoutCheckTick {};
  struct PowerOnCommand {
    std::uint8_t peer_id;
  };
  struct PowerOffCommand {
    std::uint8_t peer_id;
  };
  struct StartPowerSequenceCommand {};
  struct StopAllCommand {};
  using Event = std::variant<HeartbeatTick, HeartbeatReceived, TimeoutCheckTick, PowerOnCommand,
                             PowerOffCommand, StartPowerSequenceCommand, StopAllCommand>;

  NetworkSupervisorTask(messaging::MainBoardMessageSenderRequirements& sender,
                        midismith::os::QueueRequirements<Event>& event_queue,
                        midismith::main_board::app::adc::AdcBoardsController<config::kMaxPeerCount>&
                            boards_controller,
                        midismith::os::UptimeProviderRequirements& clock,
                        std::uint32_t peer_timeout_ms) noexcept
      : sender_(sender),
        event_queue_(event_queue),
        boards_controller_(boards_controller),
        peer_registry_(peer_timeout_ms, boards_controller),
        clock_(clock) {}

  void Run() noexcept;

 private:
  messaging::MainBoardMessageSenderRequirements& sender_;
  midismith::os::QueueRequirements<Event>& event_queue_;
  midismith::main_board::app::adc::AdcBoardsController<config::kMaxPeerCount>& boards_controller_;
  midismith::protocol::PeerRegistry<config::kMaxPeerCount> peer_registry_;
  midismith::os::UptimeProviderRequirements& clock_;
};

}  // namespace midismith::main_board::app::supervisor
