#include "app/messaging/adc_inbound_command_handler.hpp"

#include "app/config/config.hpp"

namespace midismith::adc_board::app::messaging {

void AdcInboundCommandHandler::OnAdcStart(const midismith::protocol::AdcStart& command) noexcept {
  (void) command;
  (void) acquisition_control_.RequestEnable();
}

void AdcInboundCommandHandler::OnAdcStop(const midismith::protocol::AdcStop& command) noexcept {
  (void) command;
  (void) acquisition_control_.RequestDisable();
}

void AdcInboundCommandHandler::OnCalibStart(
    const midismith::protocol::CalibStart& command) noexcept {
  (void) command;
  (void) acquisition_control_.RequestCalibrationStart();
  rest_phase_timer_.Start(midismith::adc_board::app::config::kCalibrationRestDurationMs);
}

void AdcInboundCommandHandler::OnDumpRequest(const midismith::protocol::DumpRequest& command,
                                             std::uint8_t /*source_node_id*/) noexcept {
  (void) command;
  (void) acquisition_control_.RequestCalibrationDataCollection();

  CalibrationArray result{};
  if (!calibration_result_queue_.Receive(
          result, midismith::adc_board::app::config::kCalibrationDumpResultTimeoutMs)) {
    return;
  }

  using StartTransfer = midismith::adc_board::app::calibration::CalibrationTask::StartTransfer;
  calibration_event_queue_.Send(StartTransfer{result}, midismith::os::kNoWait);
}

}  // namespace midismith::adc_board::app::messaging
