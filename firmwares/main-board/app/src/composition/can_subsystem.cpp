#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/messaging/main_board_can_message_sender.hpp"
#include "app/messaging/main_board_inbound_heartbeat_handler.hpp"
#include "app/messaging/main_board_inbound_sensor_event_logging_handler.hpp"
#include "app/messaging/main_board_inbound_sensor_event_midi_handler.hpp"
#include "bsp/can/can_bus_stats.hpp"
#include "bsp/can/fdcan_transceiver.hpp"
#include "can-broker/can_task.hpp"
#include "fdcan.h"
#include "logging/logger_requirements.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "protocol-can/can_filter_factory.hpp"
#include "protocol-can/can_to_protocol_adapter.hpp"
#include "protocol/handlers/inbound_message_dispatcher.hpp"

namespace midismith::main_board::app::composition {

namespace {

void CanTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::can_broker::CanTask*>(ctx)->Run();
  }
}

}  // namespace

CanContext CreateCanSubsystem(
    midismith::logging::LoggerRequirements& logger,
    midismith::piano_controller::PianoRequirements& piano,
    const midismith::main_board::domain::config::KeymapLookupRequirements& keymap_lookup,
    SupervisorContext& supervisor_ctx) noexcept {
  static midismith::os::Queue<midismith::bsp::can::FdcanFrame,
                              app::config::CAN_RECEIVE_QUEUE_CAPACITY>
      receive_queue;
  static midismith::bsp::can::CanBusStats stats(reinterpret_cast<void*>(&hfdcan1));
  static midismith::bsp::can::FdcanTransceiver transceiver(reinterpret_cast<void*>(&hfdcan1),
                                                           receive_queue, stats);
  static midismith::main_board::app::messaging::MainBoardInboundSensorEventLoggingHandler
      inbound_logging_handler(logger);
  static midismith::main_board::app::messaging::MainBoardInboundSensorEventMidiHandler
      inbound_midi_handler(piano, keymap_lookup);
  static midismith::main_board::app::messaging::MainBoardInboundHeartbeatHandler
      inbound_heartbeat_handler(supervisor_ctx.event_queue);
  static midismith::protocol::handlers::InboundMessageDispatcher inbound_dispatcher(
      inbound_logging_handler, inbound_midi_handler, inbound_heartbeat_handler);
  static midismith::protocol_can::CanToProtocolAdapter inbound_adapter(inbound_dispatcher);
  static midismith::can_broker::CanTask can_task(receive_queue, inbound_adapter);

  const auto filter_set = midismith::protocol_can::CanFilterFactory::MakeMainFilters();

  if (!transceiver.ConfigureReceiveFilters(filter_set.filters)) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: filter configuration failed");
  }
  if (!transceiver.ConfigureGlobalRejectFilter()) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: global filter configuration failed");
  }
  if (!transceiver.Start()) {
    logger.logf(midismith::logging::Level::Error, "FDCAN: start failed");
  }

  (void) midismith::os::Task::create("CanTask", CanTaskEntry, &can_task,
                                     app::config::CAN_TASK_STACK_BYTES,
                                     app::config::CAN_TASK_PRIORITY);

  static midismith::main_board::app::messaging::MainBoardCanMessageSender message_sender(
      transceiver);

  return CanContext{transceiver, stats, message_sender, inbound_adapter};
}

}  // namespace midismith::main_board::app::composition
