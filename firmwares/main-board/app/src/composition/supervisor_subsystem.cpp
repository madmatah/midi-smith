#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/supervisor/network_supervisor_task.hpp"
#include "os/os_uptime_provider.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"
#include "protocol/peer_registry_observer_requirements.hpp"
#include "protocol/peer_status.hpp"

namespace midismith::main_board::app::composition {

namespace {

using Event = midismith::main_board::app::supervisor::NetworkSupervisorTask::Event;
using Task = midismith::main_board::app::supervisor::NetworkSupervisorTask;

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

class NullPeerRegistryObserver final
    : public midismith::protocol::PeerRegistryObserverRequirements {
 public:
  void OnPeerChanged(std::uint8_t /*node_id*/,
                     midismith::protocol::PeerStatus /*status*/) noexcept override {}
};

}  // namespace

SupervisorContext CreateSupervisorContext() noexcept {
  static midismith::os::Queue<Event, 4> event_queue;
  return SupervisorContext{event_queue};
}

void CreateSupervisorSubsystem(messaging::MainBoardMessageSenderRequirements& sender,
                               SupervisorContext& ctx) noexcept {
  static NullPeerRegistryObserver peer_registry_observer;
  static midismith::os::OsUptimeProvider uptime_provider;
  static Task supervisor_task(sender, ctx.event_queue, peer_registry_observer, uptime_provider,
                              app::config::kHeartbeatTimeoutMs);
  static midismith::os::Timer heartbeat_timer(OnHeartbeatTick, &ctx.event_queue);
  static midismith::os::Timer timeout_check_timer(OnTimeoutCheckTick, &ctx.event_queue);

  (void) midismith::os::Task::create("SupervisorTask", SupervisorTaskEntry, &supervisor_task,
                                     app::config::SUPERVISOR_TASK_STACK_BYTES,
                                     app::config::SUPERVISOR_TASK_PRIORITY);
  heartbeat_timer.Start(app::config::kHeartbeatPeriodMs);
  timeout_check_timer.Start(app::config::kTimeoutCheckPeriodMs);
}

}  // namespace midismith::main_board::app::composition
