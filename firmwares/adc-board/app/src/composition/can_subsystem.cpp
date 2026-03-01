#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/messaging/adc_inbound_command_handler.hpp"
#include "bsp/can/can_bus_stats.hpp"
#include "bsp/can/fdcan_transceiver.hpp"
#include "can-broker/can_task.hpp"
#include "fdcan.h"
#include "logging/logger_requirements.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "protocol-can/can_to_protocol_adapter.hpp"
#include "protocol/handlers/inbound_message_dispatcher.hpp"

namespace midismith::adc_board::app::composition {

namespace {

void CanTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::can_broker::CanTask*>(ctx)->Run();
  }
}

}  // namespace

CanContext CreateCanSubsystem(midismith::logging::LoggerRequirements& logger,
                              midismith::adc_board::app::analog::AcquisitionControlRequirements&
                                  acquisition_control) noexcept {
  static midismith::os::Queue<midismith::bsp::can::FdcanFrame,
                              app::config::CAN_RECEIVE_QUEUE_CAPACITY>
      receive_queue;
  static midismith::bsp::can::CanBusStats stats(reinterpret_cast<void*>(&hfdcan1));
  static midismith::bsp::can::FdcanTransceiver transceiver(reinterpret_cast<void*>(&hfdcan1),
                                                           receive_queue, stats);
  static midismith::adc_board::app::messaging::AdcInboundCommandHandler inbound_command_handler(
      acquisition_control);
  static midismith::protocol::handlers::InboundMessageDispatcher inbound_dispatcher(
      inbound_command_handler);
  static midismith::protocol_can::CanToProtocolAdapter inbound_adapter(inbound_dispatcher);
  static midismith::can_broker::CanTask can_task(receive_queue, inbound_adapter);

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

  return CanContext{transceiver, stats};
}

}  // namespace midismith::adc_board::app::composition
