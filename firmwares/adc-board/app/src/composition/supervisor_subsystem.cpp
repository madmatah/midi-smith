#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "os/os_uptime_provider.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"
#include "protocol/peer_monitor_observer_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace midismith::adc_board::app::composition {

namespace {

using Event = midismith::adc_board::app::supervisor::AdcSupervisorTask::Event;
using Task = midismith::adc_board::app::supervisor::AdcSupervisorTask;

void SupervisorTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<Task*>(ctx)->Run();
  }
}

void OnHeartbeatTick(void* context) noexcept {
  auto* queue = static_cast<midismith::os::QueueRequirements<Event>*>(context);
  queue->Send(Event{Task::HeartbeatTick{}}, midismith::os::kNoWait);
}

void OnTimeoutCheckTick(void* context) noexcept {
  auto* queue = static_cast<midismith::os::QueueRequirements<Event>*>(context);
  queue->Send(Event{Task::TimeoutCheckTick{}}, midismith::os::kNoWait);
}

class NullPeerMonitorObserver final : public midismith::protocol::PeerMonitorObserverRequirements {
 public:
  void OnPeerHeartbeat(midismith::protocol::DeviceState /*device_state*/) noexcept override {}
  void OnPeerLost() noexcept override {}
};

}  // namespace

SupervisorContext CreateSupervisorContext() noexcept {
  static midismith::os::Queue<Event, 16> event_queue;
  return SupervisorContext{event_queue};
}

void CreateSupervisorSubsystem(messaging::AdcBoardMessageSenderRequirements& sender,
                               analog::AcquisitionStateRequirements& acquisition_state,
                               SupervisorContext& ctx) noexcept {
  static NullPeerMonitorObserver peer_state_observer;
  static midismith::os::OsUptimeProvider uptime_provider;
  static Task supervisor_task(sender, acquisition_state, ctx.event_queue, peer_state_observer,
                              uptime_provider, app::config::kHeartbeatTimeoutMs);
  static midismith::os::Timer heartbeat_timer(OnHeartbeatTick, &ctx.event_queue);
  static midismith::os::Timer timeout_check_timer(OnTimeoutCheckTick, &ctx.event_queue);

  (void) midismith::os::Task::create("SupervisorTask", SupervisorTaskEntry, &supervisor_task,
                                     app::config::SUPERVISOR_TASK_STACK_BYTES,
                                     app::config::SUPERVISOR_TASK_PRIORITY);
  heartbeat_timer.Start(app::config::kHeartbeatPeriodMs);
  timeout_check_timer.Start(app::config::kTimeoutCheckPeriodMs);
}

}  // namespace midismith::adc_board::app::composition
