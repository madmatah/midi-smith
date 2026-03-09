#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/shell/adc_boards_control_requirements.hpp"
#include "app/supervisor/network_supervisor_task.hpp"
#include "domain/adc/adc_board_power_switch_requirements.hpp"
#include "domain/adc/adc_boards_controller.hpp"
#include "os-types/queue_requirements.hpp"
#include "os/os_uptime_provider.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"

namespace midismith::main_board::app::composition {

namespace {

using Event = midismith::main_board::app::supervisor::NetworkSupervisorTask::Event;
using Task = midismith::main_board::app::supervisor::NetworkSupervisorTask;
using BoardsController =
    midismith::main_board::domain::adc::AdcBoardsController<app::config::kMaxPeerCount>;

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

class NullAdcBoardPowerSwitch final
    : public midismith::main_board::domain::adc::AdcBoardPowerSwitchRequirements {
 public:
  void PowerOn(std::uint8_t /*peer_id*/) noexcept override {}
  void PowerOff(std::uint8_t /*peer_id*/) noexcept override {}
};

class SupervisorCommandProxy final
    : public midismith::main_board::app::shell::AdcBoardsControlRequirements {
 public:
  SupervisorCommandProxy(midismith::os::QueueRequirements<Event>& queue,
                         BoardsController& boards_controller) noexcept
      : queue_(queue), boards_controller_(boards_controller) {}

  void StartPowerSequence() noexcept override {
    queue_.Send(Event{Task::StartPowerSequenceCommand{}}, midismith::os::kNoWait);
  }

  void PowerOn(std::uint8_t peer_id) noexcept override {
    queue_.Send(Event{Task::PowerOnCommand{peer_id}}, midismith::os::kNoWait);
  }

  void PowerOff(std::uint8_t peer_id) noexcept override {
    queue_.Send(Event{Task::PowerOffCommand{peer_id}}, midismith::os::kNoWait);
  }

  [[nodiscard]] midismith::main_board::domain::adc::AdcBoardState board_state(
      std::uint8_t peer_id) const noexcept override {
    return boards_controller_.board_state(peer_id);
  }

 private:
  midismith::os::QueueRequirements<Event>& queue_;
  BoardsController& boards_controller_;
};

}  // namespace

SupervisorContext CreateSupervisorContext() noexcept {
  static midismith::os::Queue<Event, 16> event_queue;
  return SupervisorContext{event_queue};
}

AdcBoardsContext CreateSupervisorSubsystem(messaging::MainBoardMessageSenderRequirements& sender,
                                           SupervisorContext& ctx) noexcept {
  static NullAdcBoardPowerSwitch power_switch;
  static midismith::os::OsUptimeProvider uptime_provider;
  static BoardsController boards_controller(power_switch, app::config::kPowerOnTimeoutMs,
                                            uptime_provider);
  static Task supervisor_task(sender, ctx.event_queue, boards_controller, uptime_provider,
                              app::config::kHeartbeatTimeoutMs);
  static SupervisorCommandProxy proxy(ctx.event_queue, boards_controller);
  static midismith::os::Timer heartbeat_timer(OnHeartbeatTick, &ctx.event_queue);
  static midismith::os::Timer timeout_check_timer(OnTimeoutCheckTick, &ctx.event_queue);

  (void) midismith::os::Task::create("SupervisorTask", SupervisorTaskEntry, &supervisor_task,
                                     app::config::SUPERVISOR_TASK_STACK_BYTES,
                                     app::config::SUPERVISOR_TASK_PRIORITY);
  heartbeat_timer.Start(app::config::kHeartbeatPeriodMs);
  timeout_check_timer.Start(app::config::kTimeoutCheckPeriodMs);

  return AdcBoardsContext{proxy};
}

}  // namespace midismith::main_board::app::composition
