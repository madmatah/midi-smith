#include "app/tasks/sensor_rtt_telemetry_task.hpp"

#include <array>
#include <cstdint>
#include <span>

#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"
#include "app/telemetry/sensor_rtt_protocol.hpp"
#include "app/telemetry/sensor_rtt_schema_frame_builder.hpp"
#include "app/telemetry/sensor_rtt_telemetry_defaults.hpp"
#include "os/queue_requirements.hpp"
#include "os/task.hpp"

namespace midismith::adc_board::app::tasks {
namespace {

constexpr std::uint32_t ClampOutputHz(std::uint32_t hz) noexcept {
  constexpr std::uint32_t kMinHz = 1u;
  const std::uint32_t kMaxHz =
      ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ;
  if (hz < kMinHz) {
    return kMinHz;
  }
  if (hz > kMaxHz) {
    return kMaxHz;
  }
  return hz;
}

}  // namespace

SensorRttTelemetryTask::SensorRttTelemetryTask(
    midismith::os::Queue<midismith::adc_board::app::telemetry::SensorRttTelemetryCommand, 4>&
        control_queue,
    midismith::adc_board::domain::sensors::SensorRegistry& registry,
    midismith::adc_board::app::analog::AcquisitionStateRequirements& adc_state,
    midismith::telemetry::TelemetrySenderRequirements& telemetry_sender,
    midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept
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
    const midismith::adc_board::app::telemetry::SensorRttTelemetryCommand& cmd) noexcept {
  if (cmd.kind == midismith::adc_board::app::telemetry::SensorRttTelemetryCommandKind::kOff) {
    capture_.ConfigureOff();
    schema_sent_ = false;
    last_schema_timestamp_us_ = 0u;
    last_data_timestamp_us_ = 0u;
    active_sensor_id_ = 0u;
    return;
  }

  if (cmd.kind == midismith::adc_board::app::telemetry::SensorRttTelemetryCommandKind::kObserve) {
    const midismith::adc_board::domain::sensors::SensorState* sensor =
        registry_.FindById(cmd.sensor_id);
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

  if (cmd.kind ==
      midismith::adc_board::app::telemetry::SensorRttTelemetryCommandKind::kSetOutputHz) {
    const std::uint32_t hz = ClampOutputHz(cmd.output_hz);
    capture_.SetOutputHz(hz);
    return;
  }
}

void SensorRttTelemetryTask::ApplyPendingCommands() noexcept {
  midismith::adc_board::app::telemetry::SensorRttTelemetryCommand cmd{};
  while (control_queue_.Receive(cmd, midismith::os::kNoWait)) {
    ApplyCommand(cmd);
  }
}

bool SensorRttTelemetryTask::ReceiveControlCommand(std::uint32_t timeout_ms) noexcept {
  midismith::adc_board::app::telemetry::SensorRttTelemetryCommand cmd{};
  if (!control_queue_.Receive(cmd, timeout_ms)) {
    return false;
  }
  ApplyCommand(cmd);
  ApplyPendingCommands();
  return true;
}

bool SensorRttTelemetryTask::IsAnalogAcquisitionEnabled() const noexcept {
  return adc_state_.GetState() == midismith::adc_board::app::analog::AcquisitionState::kEnabled;
}

bool SensorRttTelemetryTask::TrySendSchemaFrameIfDue(
    std::uint32_t schema_interval_us, std::span<std::uint8_t> schema_frame_bytes) noexcept {
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
      midismith::adc_board::app::telemetry::BuildSensorRttSchemaFrame(
          active_sensor_id_, schema_timestamp_us, schema_frame_bytes);
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
  const auto* frame_ptr =
      static_cast<const midismith::adc_board::app::telemetry::SensorRttDataFrame*>(nullptr);
  std::size_t available_frames = 0u;

  frame_ptr = capture_.PeekContiguousFrames(max_frames_per_write, available_frames);
  if (frame_ptr == nullptr || available_frames == 0u) {
    return false;
  }

  const std::size_t bytes_to_write =
      available_frames * sizeof(midismith::adc_board::app::telemetry::SensorRttDataFrame);
  const auto bytes = std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(frame_ptr),
                                                   bytes_to_write);

  const std::size_t written = telemetry_sender_.Send(bytes);
  if (written == 0u) {
    return false;
  }

  const std::size_t frames_written =
      written / sizeof(midismith::adc_board::app::telemetry::SensorRttDataFrame);
  if (frames_written == 0u) {
    return false;
  }

  last_data_timestamp_us_ = frame_ptr[frames_written - 1u].header.timestamp_us;
  capture_.ConsumeFrames(frames_written);
  return true;
}

void SensorRttTelemetryTask::run() noexcept {
  capture_.ConfigureOff();
  capture_.SetOutputHz(::midismith::adc_board::app::telemetry::DefaultSensorRttTelemetryOutputHz());

  constexpr std::size_t kMaxFramesPerWrite = 80u;
  static_assert(kMaxFramesPerWrite > 0u);
  constexpr std::uint32_t kSchemaIntervalUs = 1'000'000u;
  constexpr std::uint32_t kEnabledIdleWaitMs = 1u;

  std::array<std::uint8_t, midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes()>
      schema_frame_bytes{};

  for (;;) {
    if (active_sensor_id_ == 0u) {
      (void) ReceiveControlCommand(midismith::os::kWaitForever);
      continue;
    }

    ApplyPendingCommands();
    if (active_sensor_id_ == 0u) {
      continue;
    }

    if (!TrySendSchemaFrameIfDue(kSchemaIntervalUs, schema_frame_bytes)) {
      (void) ReceiveControlCommand(kEnabledIdleWaitMs);
      continue;
    }

    if (!IsAnalogAcquisitionEnabled()) {
      (void) ReceiveControlCommand(kEnabledIdleWaitMs);
      continue;
    }

    if (!TrySendCapturedDataFrames(kMaxFramesPerWrite)) {
      (void) ReceiveControlCommand(kEnabledIdleWaitMs);
    }
  }
}

bool SensorRttTelemetryTask::start() noexcept {
  return midismith::os::Task::create(
      "SensorRtt", SensorRttTelemetryTask::entry, this,
      midismith::adc_board::app::config::SENSOR_RTT_TELEMETRY_TASK_STACK_BYTES,
      midismith::adc_board::app::config::SENSOR_RTT_TELEMETRY_TASK_PRIORITY);
}

}  // namespace midismith::adc_board::app::tasks
