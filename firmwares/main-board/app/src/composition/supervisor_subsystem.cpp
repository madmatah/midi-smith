#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/supervisor/network_supervisor_task.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"

namespace midismith::main_board::app::composition {

namespace {

using Event = midismith::main_board::app::supervisor::NetworkSupervisorTask::Event;

void SupervisorTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::main_board::app::supervisor::NetworkSupervisorTask*>(ctx)->Run();
  }
}

void OnHeartbeatTick(void* ctx) noexcept {
  auto* queue = static_cast<midismith::os::Queue<Event, 4>*>(ctx);
  queue->Send(Event{midismith::main_board::app::supervisor::NetworkSupervisorTask::HeartbeatTick{}},
              midismith::os::kNoWait);
}

}  // namespace

void CreateSupervisorSubsystem(messaging::MainBoardMessageSenderRequirements& sender) noexcept {
  static midismith::os::Queue<Event, 4> event_queue;
  static midismith::main_board::app::supervisor::NetworkSupervisorTask supervisor_task(sender,
                                                                                       event_queue);
  static midismith::os::Timer heartbeat_timer(OnHeartbeatTick, &event_queue);

  (void) midismith::os::Task::create("SupervisorTask", SupervisorTaskEntry, &supervisor_task,
                                     app::config::SUPERVISOR_TASK_STACK_BYTES,
                                     app::config::SUPERVISOR_TASK_PRIORITY);
  heartbeat_timer.Start(app::config::kHeartbeatPeriodMs);
}

}  // namespace midismith::main_board::app::composition
