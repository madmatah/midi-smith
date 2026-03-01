#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/messaging/main_board_can_message_sender.hpp"
#include "bsp/can/can_bus_stats.hpp"
#include "bsp/can/fdcan_transceiver.hpp"
#include "can-broker/can_task.hpp"
#include "fdcan.h"
#include "logging/logger_requirements.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"

namespace midismith::main_board::app::composition {

namespace {

class DiscardingCanFrameHandler final : public midismith::can_broker::CanFrameHandlerRequirements {
 public:
  void Handle(const midismith::bsp::can::FdcanFrame&) noexcept override {}
};

void CanTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::can_broker::CanTask*>(ctx)->Run();
  }
}

}  // namespace

CanContext CreateCanSubsystem(midismith::logging::LoggerRequirements& logger) noexcept {
  static midismith::os::Queue<midismith::bsp::can::FdcanFrame,
                              app::config::CAN_RECEIVE_QUEUE_CAPACITY>
      receive_queue;
  static midismith::bsp::can::CanBusStats stats(reinterpret_cast<void*>(&hfdcan1));
  static midismith::bsp::can::FdcanTransceiver transceiver(reinterpret_cast<void*>(&hfdcan1),
                                                           receive_queue, stats);
  static DiscardingCanFrameHandler discarding_handler;
  static midismith::can_broker::CanTask can_task(receive_queue, discarding_handler);

  constexpr midismith::bsp::can::FdcanFilterConfig kAcceptAllFilter = {
      .filter_index = 0,
      .id = 0x000,
      .id_mask = 0x000,
  };

  if (!transceiver.ConfigureReceiveFilter(kAcceptAllFilter)) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: filter configuration failed");
  }
  if (!transceiver.Start()) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: start failed");
  }

  (void) midismith::os::Task::create("CanTask", CanTaskEntry, &can_task,
                                     app::config::CAN_TASK_STACK_BYTES,
                                     app::config::CAN_TASK_PRIORITY);

  static midismith::main_board::app::messaging::MainBoardCanMessageSender message_sender(
      transceiver);

  return CanContext{transceiver, stats, message_sender};
}

}  // namespace midismith::main_board::app::composition
