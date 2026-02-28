#pragma once

#include "bsp-types/can/fdcan_frame.hpp"
#include "can-broker/can_frame_handler_requirements.hpp"
#include "os-types/queue_requirements.hpp"

namespace midismith::can_broker {

class CanTask {
 public:
  CanTask(midismith::os::QueueRequirements<midismith::bsp::can::FdcanFrame>& receive_queue,
          CanFrameHandlerRequirements& handler) noexcept;

  void Run() noexcept;

 private:
  midismith::os::QueueRequirements<midismith::bsp::can::FdcanFrame>& receive_queue_;
  CanFrameHandlerRequirements& handler_;
};

}  // namespace midismith::can_broker
