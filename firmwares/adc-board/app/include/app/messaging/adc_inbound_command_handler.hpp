#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcInboundCommandHandler final {
 public:
  explicit AdcInboundCommandHandler(
      midismith::adc_board::app::analog::AcquisitionControlRequirements&
          acquisition_control) noexcept
      : acquisition_control_(acquisition_control) {}

  void OnAdcStart(const midismith::protocol::AdcStart& command) noexcept;
  void OnAdcStop(const midismith::protocol::AdcStop& command) noexcept;

 private:
  midismith::adc_board::app::analog::AcquisitionControlRequirements& acquisition_control_;
};

}  // namespace midismith::adc_board::app::messaging
