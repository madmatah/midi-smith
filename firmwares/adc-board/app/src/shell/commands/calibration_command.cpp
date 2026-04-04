#include "app/shell/commands/calibration_command.hpp"

#include <cstdint>
#include <string_view>

#include "app/config/sensors.hpp"
#include "calibration/sensor_calibration_formatter.hpp"

namespace midismith::adc_board::app::shell::commands {
namespace {

constexpr std::uint32_t kPauseAfterSensorLineMs = 10u;

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: calibration show\r\n");
}

}  // namespace

CalibrationCommand::CalibrationCommand(
    midismith::adc_board::app::calibration::CalibrationQueryRequirements& query,
    BlockingDelayRequirements& blocking_delay) noexcept
    : query_(query), blocking_delay_(blocking_delay) {}

void CalibrationCommand::PauseToAllowUartTransmitBufferDrain() noexcept {
  blocking_delay_.DelayMs(kPauseAfterSensorLineMs);
}

void CalibrationCommand::Run(int argc, char** argv,
                             midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2 || argv == nullptr || argv[1] == nullptr || std::string_view(argv[1]) != "show") {
    WriteUsage(out);
    return;
  }

  for (std::uint8_t index = 0; index < midismith::adc_board::app::config::sensors::kSensorCount;
       ++index) {
    const std::uint8_t sensor_id = midismith::adc_board::app::config::sensors::kSensorIds[index];
    midismith::calibration::WriteSensorCalibrationLine(out, sensor_id,
                                                       query_.sensor_calibration(index));
    PauseToAllowUartTransmitBufferDrain();
  }
}

}  // namespace midismith::adc_board::app::shell::commands
