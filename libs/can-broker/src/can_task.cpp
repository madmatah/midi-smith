#include "can-broker/can_task.hpp"

#include "os-types/queue_requirements.hpp"

namespace midismith::can_broker {

CanTask::CanTask(midismith::os::QueueRequirements<midismith::bsp::can::FdcanFrame>& receive_queue,
                 CanFrameHandlerRequirements& handler) noexcept
    : receive_queue_(receive_queue), handler_(handler) {}

void CanTask::Run() noexcept {
  midismith::bsp::can::FdcanFrame frame{};
  while (receive_queue_.Receive(frame, midismith::os::kWaitForever)) {
    handler_.Handle(frame);
  }
}

}  // namespace midismith::can_broker
