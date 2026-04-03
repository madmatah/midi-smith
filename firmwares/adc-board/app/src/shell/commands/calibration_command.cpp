#include "app/shell/commands/calibration_command.hpp"

#include <cstdint>
#include <string_view>

#include "app/config/sensors.hpp"
#include "io/stream_format.hpp"

namespace midismith::adc_board::app::shell::commands {
namespace {

void WriteUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: calibration status\r\n");
}

void WriteSensorCalibrationLine(
    midismith::io::WritableStreamRequirements& out, std::uint8_t sensor_id,
    const midismith::calibration::SensorCalibration& calibration) noexcept {
  out.Write("sensor[");
  midismith::io::WriteUint8(out, sensor_id);
  out.Write("]  rest_mA=");
  midismith::io::WriteFloat<3>(out, calibration.rest_current_ma);
  out.Write("  strike_mA=");
  midismith::io::WriteFloat<3>(out, calibration.strike_current_ma);
  out.Write("  rest_mm=");
  midismith::io::WriteFloat<3>(out, calibration.rest_distance_mm);
  out.Write("  strike_mm=");
  midismith::io::WriteFloat<3>(out, calibration.strike_distance_mm);
  out.Write("\r\n");
}

}  // namespace

void CalibrationCommand::Run(int argc, char** argv,
                             midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2 || argv == nullptr || argv[1] == nullptr || std::string_view(argv[1]) != "status") {
    WriteUsage(out);
    return;
  }

  for (std::uint8_t index = 0; index < midismith::adc_board::app::config::sensors::kSensorCount;
       ++index) {
    const std::uint8_t sensor_id = midismith::adc_board::app::config::sensors::kSensorIds[index];
    WriteSensorCalibrationLine(out, sensor_id, query_.sensor_calibration(index));
  }
}

}  // namespace midismith::adc_board::app::shell::commands
