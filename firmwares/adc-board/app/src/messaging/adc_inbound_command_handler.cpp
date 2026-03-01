#include "app/messaging/adc_inbound_command_handler.hpp"

namespace midismith::adc_board::app::messaging {

void AdcInboundCommandHandler::OnAdcStart(const midismith::protocol::AdcStart& command) noexcept {
  (void) command;
  (void) acquisition_control_.RequestEnable();
}

void AdcInboundCommandHandler::OnAdcStop(const midismith::protocol::AdcStop& command) noexcept {
  (void) command;
  (void) acquisition_control_.RequestDisable();
}

}  // namespace midismith::adc_board::app::messaging
