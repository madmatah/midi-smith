#pragma once

#include <variant>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::main_board::app::supervisor {

class NetworkSupervisorTask {
 public:
  struct HeartbeatTick {};
  using Event = std::variant<HeartbeatTick>;

  NetworkSupervisorTask(messaging::MainBoardMessageSenderRequirements& sender,
                        os::QueueRequirements<Event>& event_queue) noexcept
      : sender_(sender), event_queue_(event_queue) {}

  void Run() noexcept;

 private:
  messaging::MainBoardMessageSenderRequirements& sender_;
  os::QueueRequirements<Event>& event_queue_;
};

}  // namespace midismith::main_board::app::supervisor
