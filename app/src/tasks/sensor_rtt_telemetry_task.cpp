#include "app/tasks/sensor_rtt_telemetry_task.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"
#include "app/telemetry/sensor_rtt_protocol.hpp"
#include "app/telemetry/sensor_rtt_telemetry_defaults.hpp"
#include "os/clock.hpp"
#include "os/task.hpp"

namespace app::Tasks {
namespace {

constexpr std::uint32_t ClampOutputHz(std::uint32_t hz) noexcept {
  constexpr std::uint32_t kMinHz = 1u;
  const std::uint32_t kMaxHz = ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
  if (hz < kMinHz) {
    return kMinHz;
  }
  if (hz > kMaxHz) {
    return kMaxHz;
  }
  return hz;
}

class ByteWriter final {
 public:
  explicit ByteWriter(std::span<std::uint8_t> out) noexcept
      : begin_(out.data()), ptr_(out.data()), end_(out.data() + out.size()) {}

  bool Skip(std::size_t bytes) noexcept {
    if (bytes > Remaining()) {
      return false;
    }
    ptr_ += bytes;
    return true;
  }

  bool WriteU8(std::uint8_t value) noexcept {
    if (Remaining() < 1u) {
      return false;
    }
    *ptr_ = value;
    ++ptr_;
    return true;
  }

  bool WriteU16LE(std::uint16_t value) noexcept {
    if (Remaining() < 2u) {
      return false;
    }
    ptr_[0] = static_cast<std::uint8_t>(value & 0xFFu);
    ptr_[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    ptr_ += 2;
    return true;
  }

  bool WriteU32LE(std::uint32_t value) noexcept {
    if (Remaining() < 4u) {
      return false;
    }
    ptr_[0] = static_cast<std::uint8_t>(value & 0xFFu);
    ptr_[1] = static_cast<std::uint8_t>((value >> 8) & 0xFFu);
    ptr_[2] = static_cast<std::uint8_t>((value >> 16) & 0xFFu);
    ptr_[3] = static_cast<std::uint8_t>((value >> 24) & 0xFFu);
    ptr_ += 4;
    return true;
  }

  bool WriteBytes(std::string_view text) noexcept {
    if (text.size() > Remaining()) {
      return false;
    }
    std::memcpy(ptr_, text.data(), text.size());
    ptr_ += text.size();
    return true;
  }

  std::size_t Size() const noexcept {
    return static_cast<std::size_t>(ptr_ - begin_);
  }

 private:
  std::size_t Remaining() const noexcept {
    return static_cast<std::size_t>(end_ - ptr_);
  }

  std::uint8_t* begin_ = nullptr;
  std::uint8_t* ptr_ = nullptr;
  std::uint8_t* end_ = nullptr;
};

std::size_t BuildSchemaFrame(std::uint8_t sensor_id, std::uint32_t timestamp_us,
                             std::span<std::uint8_t> out) noexcept {
  ByteWriter w(out);
  if (!w.Skip(sizeof(app::telemetry::SensorRttFrameHeader))) {
    return 0u;
  }

  constexpr std::uint8_t kMetricCount = 5u;
  constexpr std::uint16_t kDataPayloadSize =
      static_cast<std::uint16_t>(sizeof(app::telemetry::SensorRttDataPayload));
  constexpr std::uint16_t kHeaderSize =
      static_cast<std::uint16_t>(sizeof(app::telemetry::SensorRttFrameHeader));

  if (!w.WriteU8(sensor_id) || !w.WriteU8(kMetricCount) || !w.WriteU16LE(kDataPayloadSize) ||
      !w.WriteU16LE(kHeaderSize) || !w.WriteU16LE(0u)) {
    return 0u;
  }

  struct MetricDef {
    std::uint8_t id;
    std::uint16_t offset_bytes;
    std::string_view name;
  };

  constexpr MetricDef kMetrics[] = {
      MetricDef{0u, 4u, "ADC (raw)"},           MetricDef{1u, 8u, "ADC (filtered)"},
      MetricDef{2u, 12u, "Current (mA)"},       MetricDef{3u, 16u, "Normalized Position"},
      MetricDef{4u, 20u, "Hammer Speed (m/s)"},
  };

  for (const MetricDef& m : kMetrics) {
    const auto name_len = static_cast<std::uint8_t>(m.name.size());
    if (!w.WriteU8(m.id) ||
        !w.WriteU8(static_cast<std::uint8_t>(app::telemetry::SensorRttValueType::kFloat32)) ||
        !w.WriteU16LE(m.offset_bytes) || !w.WriteU8(name_len) || !w.WriteBytes(m.name)) {
      return 0u;
    }
  }

  const std::size_t total_bytes = w.Size();
  if (total_bytes < sizeof(app::telemetry::SensorRttFrameHeader)) {
    return 0u;
  }

  app::telemetry::SensorRttFrameHeader header{};
  header.magic = app::telemetry::kSensorRttMagic;
  header.version = app::telemetry::kSensorRttVersion;
  header.kind = static_cast<std::uint8_t>(app::telemetry::SensorRttFrameKind::kSchema);
  header.payload_size_bytes =
      static_cast<std::uint16_t>(total_bytes - sizeof(app::telemetry::SensorRttFrameHeader));
  header.seq = 0u;
  header.timestamp_us = timestamp_us;

  std::memcpy(out.data(), &header, sizeof(header));
  return total_bytes;
}

}  // namespace

SensorRttTelemetryTask::SensorRttTelemetryTask(
    os::Queue<app::telemetry::SensorRttTelemetryCommand, 4>& control_queue,
    domain::sensors::SensorRegistry& registry, app::analog::AcquisitionStateRequirements& adc_state,
    app::telemetry::TelemetrySenderRequirements& telemetry_sender,
    app::telemetry::SensorRttStreamCapture& capture) noexcept
    : control_queue_(control_queue),
      registry_(registry),
      adc_state_(adc_state),
      telemetry_sender_(telemetry_sender),
      capture_(capture) {}

void SensorRttTelemetryTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<SensorRttTelemetryTask*>(ctx)->run();
}

void SensorRttTelemetryTask::ApplyCommand(
    const app::telemetry::SensorRttTelemetryCommand& cmd) noexcept {
  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kOff) {
    capture_.ConfigureOff();
    schema_sent_ = false;
    last_schema_timestamp_us_ = 0u;
    last_data_timestamp_us_ = 0u;
    active_sensor_id_ = 0u;
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kObserve) {
    const domain::sensors::SensorState* sensor = registry_.FindById(cmd.sensor_id);
    if (sensor == nullptr) {
      capture_.ConfigureOff();
      schema_sent_ = false;
      last_schema_timestamp_us_ = 0u;
      last_data_timestamp_us_ = 0u;
      active_sensor_id_ = 0u;
      return;
    }

    capture_.ConfigureObserve(cmd.sensor_id);
    schema_sent_ = false;
    last_schema_timestamp_us_ = 0u;
    last_data_timestamp_us_ = 0u;
    active_sensor_id_ = cmd.sensor_id;
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kSetOutputHz) {
    const std::uint32_t hz = ClampOutputHz(cmd.output_hz);
    capture_.SetOutputHz(hz);
    return;
  }
}

void SensorRttTelemetryTask::ApplyPendingCommands() noexcept {
  app::telemetry::SensorRttTelemetryCommand cmd{};
  while (control_queue_.Receive(cmd, os::kNoWait)) {
    ApplyCommand(cmd);
  }
}

bool SensorRttTelemetryTask::IsAnalogAcquisitionEnabled() const noexcept {
  return adc_state_.GetState() == app::analog::AcquisitionState::kEnabled;
}

bool SensorRttTelemetryTask::TrySendSchemaFrameIfDue(
    std::uint32_t schema_interval_us, std::array<std::uint8_t, 256u>& schema_frame_bytes) noexcept {
  if (active_sensor_id_ == 0u) {
    return true;
  }

  const bool schema_due_by_interval =
      schema_sent_ && last_data_timestamp_us_ != 0u &&
      static_cast<std::uint32_t>(last_data_timestamp_us_ - last_schema_timestamp_us_) >=
          schema_interval_us;
  const bool schema_due = !schema_sent_ || schema_due_by_interval;
  if (!schema_due) {
    return true;
  }

  const std::uint32_t schema_timestamp_us = last_data_timestamp_us_;
  const std::size_t schema_size_bytes =
      BuildSchemaFrame(active_sensor_id_, schema_timestamp_us, schema_frame_bytes);
  if (schema_size_bytes == 0u) {
    return true;
  }

  const auto schema_bytes =
      std::span<const std::uint8_t>(schema_frame_bytes.data(), schema_size_bytes);
  const std::size_t written = telemetry_sender_.Send(schema_bytes);
  if (written == 0u) {
    return false;
  }

  schema_sent_ = true;
  last_schema_timestamp_us_ = schema_timestamp_us;
  return true;
}

bool SensorRttTelemetryTask::TrySendCapturedDataFrames(std::size_t max_frames_per_write) noexcept {
  const auto* frame_ptr = static_cast<const app::telemetry::SensorRttDataFrame*>(nullptr);
  std::size_t available_frames = 0u;

  frame_ptr = capture_.PeekContiguousFrames(max_frames_per_write, available_frames);
  if (frame_ptr == nullptr || available_frames == 0u) {
    return false;
  }

  const std::size_t bytes_to_write = available_frames * sizeof(app::telemetry::SensorRttDataFrame);
  const auto bytes = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(frame_ptr),
                                                   bytes_to_write);

  const std::size_t written = telemetry_sender_.Send(bytes);
  if (written == 0u) {
    return false;
  }

  const std::size_t frames_written = written / sizeof(app::telemetry::SensorRttDataFrame);
  if (frames_written == 0u) {
    return false;
  }

  last_data_timestamp_us_ = frame_ptr[frames_written - 1u].header.timestamp_us;
  capture_.ConsumeFrames(frames_written);
  return true;
}

void SensorRttTelemetryTask::run() noexcept {
  capture_.ConfigureOff();
  capture_.SetOutputHz(::app::telemetry::DefaultSensorRttTelemetryOutputHz());

  constexpr std::size_t kMaxFramesPerWrite = 80u;
  static_assert(kMaxFramesPerWrite > 0u);
  constexpr std::uint32_t kSchemaIntervalUs = 1'000'000u;

  std::array<std::uint8_t, 256u> schema_frame_bytes{};

  for (;;) {
    ApplyPendingCommands();

    if (!TrySendSchemaFrameIfDue(kSchemaIntervalUs, schema_frame_bytes)) {
      os::Clock::delay_ms(1);
      continue;
    }

    if (!IsAnalogAcquisitionEnabled()) {
      os::Clock::delay_ms(1);
      continue;
    }

    if (!TrySendCapturedDataFrames(kMaxFramesPerWrite)) {
      os::Clock::delay_ms(1);
    }
  }
}

bool SensorRttTelemetryTask::start() noexcept {
  return os::Task::create("SensorRtt", SensorRttTelemetryTask::entry, this,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_STACK_BYTES,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_PRIORITY);
}

}  // namespace app::Tasks
