#pragma once

#include "app/analog/acquisition_control_requirements.hpp"
#include "app/calibration/calibration_task.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol/messages.hpp"

namespace midismith::adc_board::app::messaging {

class AdcInboundCommandHandler final {
 public:
  using CalibrationArray =
      midismith::adc_board::app::analog::AcquisitionControlRequirements::CalibrationArray;
  using CalibrationEvent = midismith::adc_board::app::calibration::CalibrationTask::Event;

  AdcInboundCommandHandler(
      midismith::adc_board::app::analog::AcquisitionControlRequirements& acquisition_control,
      midismith::os::TimerRequirements& rest_phase_timer,
      midismith::os::QueueRequirements<CalibrationEvent>& calibration_event_queue,
      midismith::os::QueueRequirements<CalibrationArray>& calibration_result_queue) noexcept
      : acquisition_control_(acquisition_control),
        rest_phase_timer_(rest_phase_timer),
        calibration_event_queue_(calibration_event_queue),
        calibration_result_queue_(calibration_result_queue) {}

  void OnAdcStart(const midismith::protocol::AdcStart& command) noexcept;
  void OnAdcStop(const midismith::protocol::AdcStop& command) noexcept;
  void OnCalibStart(const midismith::protocol::CalibStart& command) noexcept;
  void OnDumpRequest(const midismith::protocol::DumpRequest& command,
                     std::uint8_t source_node_id) noexcept;

 private:
  midismith::adc_board::app::analog::AcquisitionControlRequirements& acquisition_control_;
  midismith::os::TimerRequirements& rest_phase_timer_;
  midismith::os::QueueRequirements<CalibrationEvent>& calibration_event_queue_;
  midismith::os::QueueRequirements<CalibrationArray>& calibration_result_queue_;
};

}  // namespace midismith::adc_board::app::messaging
