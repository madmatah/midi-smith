#pragma once

#include <variant>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "os-types/queue_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::supervisor {

class AdcSupervisorTask {
 public:
  struct HeartbeatTick {};
  using Event = std::variant<HeartbeatTick>;

  AdcSupervisorTask(messaging::AdcBoardMessageSenderRequirements& sender,
                    analog::AcquisitionStateRequirements& acquisition_state,
                    os::QueueRequirements<Event>& event_queue) noexcept
      : sender_(sender), acquisition_state_(acquisition_state), event_queue_(event_queue) {}

  void Run() noexcept;

 private:
  protocol::DeviceState CurrentDeviceState() const noexcept;

  messaging::AdcBoardMessageSenderRequirements& sender_;
  analog::AcquisitionStateRequirements& acquisition_state_;
  os::QueueRequirements<Event>& event_queue_;
};

}  // namespace midismith::adc_board::app::supervisor
