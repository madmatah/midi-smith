#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"

namespace midismith::adc_board::app::composition {

namespace {

using Event = midismith::adc_board::app::supervisor::AdcSupervisorTask::Event;

void SupervisorTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::adc_board::app::supervisor::AdcSupervisorTask*>(ctx)->Run();
  }
}

void OnHeartbeatTick(void* ctx) noexcept {
  auto* queue = static_cast<midismith::os::Queue<Event, 4>*>(ctx);
  queue->Send(Event{midismith::adc_board::app::supervisor::AdcSupervisorTask::HeartbeatTick{}},
              midismith::os::kNoWait);
}

}  // namespace

void CreateSupervisorSubsystem(messaging::AdcBoardMessageSenderRequirements& sender,
                               analog::AcquisitionStateRequirements& acquisition_state) noexcept {
  static midismith::os::Queue<Event, 4> event_queue;
  static midismith::adc_board::app::supervisor::AdcSupervisorTask supervisor_task(
      sender, acquisition_state, event_queue);
  static midismith::os::Timer heartbeat_timer(OnHeartbeatTick, &event_queue);

  (void) midismith::os::Task::create("SupervisorTask", SupervisorTaskEntry, &supervisor_task,
                                     app::config::SUPERVISOR_TASK_STACK_BYTES,
                                     app::config::SUPERVISOR_TASK_PRIORITY);
  heartbeat_timer.Start(app::config::kHeartbeatPeriodMs);
}

}  // namespace midismith::adc_board::app::composition
