#include "app/shell/commands/sensor_rtt_command.hpp"

#include <charconv>
#include <cstdint>
#include <string_view>
#include <system_error>

#include "app/config/analog_acquisition.hpp"

namespace app::shell::commands {
namespace {

std::string_view Arg(int argc, char** argv, int index) noexcept {
  if (argv == nullptr) {
    return {};
  }
  if (index < 0 || index >= argc) {
    return {};
  }
  if (argv[index] == nullptr) {
    return {};
  }
  return std::string_view(argv[index]);
}

void WriteUsage(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("usage: sensor_rtt <id> [adc|adc_filtered|current|position]\r\n");
  out.Write("       default metric is position\r\n");
  out.Write("       sensor_rtt freq [hz|max]\r\n");
  out.Write("       sensor_rtt off\r\n");
  out.Write("       sensor_rtt status\r\n");
}

void WriteRejected(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: request rejected\r\n");
}

void WriteOk(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("ok\r\n");
}

void WriteUnknownSensorId(domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("error: unknown sensor id\r\n");
}

bool ParseUint32(std::string_view text, std::uint32_t& out_value) noexcept {
  if (text.empty()) {
    return false;
  }
  std::uint32_t value = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto r = std::from_chars(begin, end, value);
  if (r.ec != std::errc() || r.ptr != end) {
    return false;
  }
  out_value = value;
  return true;
}

void WriteUint32(domain::io::WritableStreamRequirements& out, std::uint32_t value) noexcept {
  char buf[16]{};
  auto r = std::to_chars(buf, buf + sizeof(buf), value);
  if (r.ec != std::errc()) {
    return;
  }
  out.Write(std::string_view(buf, static_cast<std::size_t>(r.ptr - buf)));
}

struct SensorRttParsedCommand {
  enum class Kind : std::uint8_t {
    kOff = 0,
    kStatus = 1,
    kObserve = 2,
    kGetFreq = 3,
    kSetFreq = 4,
  };

  Kind kind{Kind::kStatus};
  std::uint8_t sensor_id{0};
  domain::sensors::SensorRttMode mode{domain::sensors::SensorRttMode::kPosition};
  std::uint32_t output_hz{0};
};

void WriteStatus(domain::io::WritableStreamRequirements& out,
                 const app::telemetry::SensorRttTelemetryStatus& status) noexcept {
  if (!status.enabled) {
    out.Write("off\r\n");
    return;
  }
  out.Write("on id=");
  WriteUint32(out, status.sensor_id);
  out.Write(" mode=");
  switch (status.mode) {
    case domain::sensors::SensorRttMode::kAdc:
      out.Write("adc");
      break;
    case domain::sensors::SensorRttMode::kAdcFiltered:
      out.Write("adc_filtered");
      break;
    case domain::sensors::SensorRttMode::kCurrent:
      out.Write("current");
      break;
    case domain::sensors::SensorRttMode::kPosition:
      out.Write("position");
      break;
  }
  out.Write(" output_hz=");
  WriteUint32(out, status.output_hz);
  out.Write(" dropped=");
  WriteUint32(out, status.dropped_frames);
  out.Write(" backlog=");
  WriteUint32(out, status.backlog_frames);
  out.Write("\r\n");
}

bool TryParseCommand(int argc, char** argv, SensorRttParsedCommand& parsed,
                     domain::io::WritableStreamRequirements& out) noexcept {
  const std::string_view op = Arg(argc, argv, 1);
  if (op.empty()) {
    WriteUsage(out);
    return false;
  }

  if (op == "off") {
    parsed.kind = SensorRttParsedCommand::Kind::kOff;
    return true;
  }

  if (op == "status") {
    parsed.kind = SensorRttParsedCommand::Kind::kStatus;
    return true;
  }

  if (op == "freq") {
    const std::string_view freq_arg = Arg(argc, argv, 2);
    if (freq_arg.empty()) {
      parsed.kind = SensorRttParsedCommand::Kind::kGetFreq;
      return true;
    }
    if (freq_arg == "max") {
      parsed.output_hz = ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
    } else if (!ParseUint32(freq_arg, parsed.output_hz) || parsed.output_hz == 0u) {
      WriteUsage(out);
      return false;
    }
    parsed.kind = SensorRttParsedCommand::Kind::kSetFreq;
    return true;
  }

  std::uint32_t sensor_id_u32 = 0;
  if (!ParseUint32(op, sensor_id_u32) || sensor_id_u32 > 255u) {
    WriteUsage(out);
    return false;
  }

  parsed.kind = SensorRttParsedCommand::Kind::kObserve;
  parsed.sensor_id = static_cast<std::uint8_t>(sensor_id_u32);

  const std::string_view mode_arg = Arg(argc, argv, 2);
  if (mode_arg.empty() || mode_arg == "position") {
    parsed.mode = domain::sensors::SensorRttMode::kPosition;
  } else if (mode_arg == "adc") {
    parsed.mode = domain::sensors::SensorRttMode::kAdc;
  } else if (mode_arg == "adc_filtered") {
    parsed.mode = domain::sensors::SensorRttMode::kAdcFiltered;
  } else if (mode_arg == "current") {
    parsed.mode = domain::sensors::SensorRttMode::kCurrent;
  } else {
    WriteUsage(out);
    return false;
  }

  return true;
}

}  // namespace

void SensorRttCommand::Run(int argc, char** argv,
                           domain::io::WritableStreamRequirements& out) noexcept {
  SensorRttParsedCommand parsed{};
  if (!TryParseCommand(argc, argv, parsed, out)) {
    return;
  }

  if (parsed.kind == SensorRttParsedCommand::Kind::kOff) {
    if (!control_.RequestOff()) {
      WriteRejected(out);
      return;
    }
    WriteOk(out);
    return;
  }

  if (parsed.kind == SensorRttParsedCommand::Kind::kStatus) {
    WriteStatus(out, control_.GetStatus());
    return;
  }

  if (parsed.kind == SensorRttParsedCommand::Kind::kGetFreq) {
    const auto status = control_.GetStatus();
    WriteUint32(out, status.output_hz);
    out.Write("\r\n");
    return;
  }

  if (parsed.kind == SensorRttParsedCommand::Kind::kSetFreq) {
    if (!control_.RequestSetOutputHz(parsed.output_hz)) {
      WriteRejected(out);
      return;
    }
    WriteOk(out);
    return;
  }

  if (registry_.FindById(parsed.sensor_id) == nullptr) {
    WriteUnknownSensorId(out);
    return;
  }

  if (!control_.RequestObserve(parsed.sensor_id, parsed.mode)) {
    WriteRejected(out);
    return;
  }
  WriteOk(out);
}

}  // namespace app::shell::commands
