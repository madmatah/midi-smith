#include "app/tasks/sensor_rtt_telemetry_task.hpp"

#include <cstdint>
#include <span>

#include "app/config/analog_acquisition.hpp"
#include "app/config/config.hpp"
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
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kObserve) {
    const domain::sensors::SensorState* sensor = registry_.FindById(cmd.sensor_id);
    if (sensor == nullptr) {
      capture_.ConfigureOff();
      return;
    }

    capture_.ConfigureObserve(cmd.sensor_id, cmd.mode);
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kSetOutputHz) {
    const std::uint32_t hz = ClampOutputHz(cmd.output_hz);
    capture_.SetOutputHz(hz);
    return;
  }
}

void SensorRttTelemetryTask::run() noexcept {
  capture_.ConfigureOff();
  capture_.SetOutputHz(::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);

  constexpr std::size_t kMaxFramesPerWrite = 80u;
  static_assert(kMaxFramesPerWrite > 0u);

  const auto* frame_ptr = static_cast<const app::telemetry::SensorRttSampleFrame*>(nullptr);
  std::size_t available_frames = 0u;

  for (;;) {
    app::telemetry::SensorRttTelemetryCommand cmd{};
    while (control_queue_.Receive(cmd, os::kNoWait)) {
      ApplyCommand(cmd);
    }

    if (adc_state_.GetState() != app::analog::AcquisitionState::kEnabled) {
      os::Clock::delay_ms(1);
      continue;
    }

    frame_ptr = capture_.PeekContiguousFrames(kMaxFramesPerWrite, available_frames);
    if (frame_ptr == nullptr || available_frames == 0u) {
      os::Clock::delay_ms(1);
      continue;
    }

    const std::size_t bytes_to_write =
        available_frames * sizeof(app::telemetry::SensorRttSampleFrame);
    const auto bytes = std::span<const std::uint8_t>(
        reinterpret_cast<const std::uint8_t*>(frame_ptr), bytes_to_write);

    const std::size_t written = telemetry_sender_.Send(bytes);
    if (written == 0u) {
      os::Clock::delay_ms(1);
      continue;
    }

    const std::size_t frames_written = written / sizeof(app::telemetry::SensorRttSampleFrame);
    if (frames_written > 0u) {
      capture_.ConsumeFrames(frames_written);
    }
  }
}

bool SensorRttTelemetryTask::start() noexcept {
  return os::Task::create("SensorRtt", SensorRttTelemetryTask::entry, this,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_STACK_BYTES,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_PRIORITY);
}

}  // namespace app::Tasks
