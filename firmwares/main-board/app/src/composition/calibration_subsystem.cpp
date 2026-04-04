#include <new>

#include "app/calibration/calibration_bulk_data_receiver.hpp"
#include "app/calibration/calibration_coordinator.hpp"
#include "app/calibration/calibration_data_server_task.hpp"
#include "app/composition/subsystems.hpp"
#include "app/config/config.hpp"
#include "app/config/flash_layout.hpp"
#include "app/shell/calibration_command.hpp"
#include "app/storage/calibration_persistent_store.hpp"
#include "bsp/board.hpp"
#include "bsp/storage/spi_flash_sector_storage.hpp"
#include "calibration/sensor_calibration_validator.hpp"
#include "domain/calibration/calibration_session.hpp"
#include "os/queue.hpp"
#include "os/task.hpp"
#include "os/timer.hpp"

namespace midismith::main_board::app::composition {

namespace {

void CalibrationDataServerTaskEntry(void* ctx) noexcept {
  if (ctx != nullptr) {
    static_cast<midismith::main_board::app::calibration::CalibrationDataServerTask*>(ctx)->Run();
  }
}

}  // namespace

CalibrationContext CreateCalibrationSubsystem(const ConfigContext& config_ctx, CanContext& can_ctx,
                                              const AdcBoardsContext& boards_ctx,
                                              CalibrationInboundContext& inbound_ctx) noexcept {
  static midismith::main_board::bsp::storage::SpiFlashSectorStorage calibration_flash_storage(
      midismith::main_board::bsp::Board::spi_flash(),
      midismith::main_board::app::config::kCalibrationSectorIndex);
  static midismith::main_board::app::storage::CalibrationPersistentStore calibration_store(
      calibration_flash_storage);
  static midismith::os::Queue<
      midismith::main_board::app::calibration::CalibrationDataServerTask::Event,
      app::config::kCalibrationServerEventQueueCapacity>
      calibration_server_event_queue;
  static midismith::os::Timer calibration_server_ack_timer(
      midismith::main_board::app::calibration::CalibrationDataServerTask::OnAckTimeout,
      &calibration_server_event_queue);
  static midismith::main_board::app::calibration::CalibrationDataServerTask calibration_server_task(
      can_ctx.message_sender, calibration_store, calibration_server_event_queue,
      calibration_server_ack_timer);

  static midismith::calibration::SensorCalibrationValidator validator(
      config::kMinimumCalibrationDeltaMa, config::kMaxValidStrikeCurrentMa);

  static midismith::main_board::app::shell::CalibrationCommand calibration_command;

  static midismith::main_board::domain::calibration::CalibrationSession calibration_session(
      config_ctx.persistent_config.active_config().data, calibration_command, validator);

  static midismith::main_board::app::calibration::CalibrationCoordinator coordinator(
      calibration_session, calibration_store, can_ctx.message_sender, boards_ctx.peer_status);

  static midismith::os::Timer rest_phase_timer(
      midismith::main_board::app::calibration::CalibrationCoordinator::OnRestPhaseTimeout,
      &coordinator);
  coordinator.SetRestPhaseTimer(rest_phase_timer);

  alignas(midismith::main_board::app::calibration::CalibrationBulkDataReceiver) static std::uint8_t
      receiver_storage[sizeof(
          midismith::main_board::app::calibration::CalibrationBulkDataReceiver)];
  auto* receiver_ptr =
      reinterpret_cast<midismith::main_board::app::calibration::CalibrationBulkDataReceiver*>(
          receiver_storage);

  static midismith::os::Timer receive_timeout_timer(
      midismith::main_board::app::calibration::CalibrationBulkDataReceiver::OnReceiveTimeout,
      receiver_ptr);

  static bool receiver_constructed = false;
  if (!receiver_constructed) {
    new (receiver_storage) midismith::main_board::app::calibration::CalibrationBulkDataReceiver(
        can_ctx.message_sender, coordinator, receive_timeout_timer);
    receiver_constructed = true;
  }
  auto& receiver = *receiver_ptr;

  coordinator.SetReceiver(receiver);
  calibration_command.SetCoordinator(coordinator);

  inbound_ctx.handler.SetCoordinator(coordinator);
  inbound_ctx.handler.SetReceiver(receiver);
  inbound_ctx.handler.SetServerEventQueue(calibration_server_event_queue);

  (void) midismith::os::Task::create("CalibrationServerTask", CalibrationDataServerTaskEntry,
                                     &calibration_server_task,
                                     app::config::CALIBRATION_SERVER_TASK_STACK_BYTES,
                                     app::config::CALIBRATION_SERVER_TASK_PRIORITY);

  return CalibrationContext{coordinator, calibration_command};
}

}  // namespace midismith::main_board::app::composition
