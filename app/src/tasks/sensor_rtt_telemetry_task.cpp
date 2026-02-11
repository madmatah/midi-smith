#include "app/tasks/sensor_rtt_telemetry_task.hpp"

#include <cstdint>

#include "app/config/config.hpp"
#include "os/task.hpp"

namespace app::Tasks {
namespace {

constexpr std::uint32_t ClampPeriodMs(std::uint32_t period_ms) noexcept {
  constexpr std::uint32_t kMinPeriodMs = 1;
  constexpr std::uint32_t kMaxPeriodMs = 1000;
  if (period_ms < kMinPeriodMs) {
    return kMinPeriodMs;
  }
  if (period_ms > kMaxPeriodMs) {
    return kMaxPeriodMs;
  }
  return period_ms;
}

}  // namespace

SensorRttTelemetryTask::SensorRttTelemetryTask(
    os::Queue<app::telemetry::SensorRttTelemetryCommand, 4>& control_queue,
    domain::sensors::SensorRegistry& registry, app::analog::AcquisitionStateRequirements& adc_state,
    app::telemetry::TelemetrySenderRequirements& telemetry_sender, volatile bool& enabled,
    volatile std::uint8_t& sensor_id, volatile domain::sensors::SensorRttMode& mode,
    volatile std::uint32_t& period_ms) noexcept
    : control_queue_(control_queue),
      registry_(registry),
      adc_state_(adc_state),
      telemetry_sender_(telemetry_sender),
      enabled_(enabled),
      sensor_id_(sensor_id),
      mode_(mode),
      period_ms_(period_ms) {}

void SensorRttTelemetryTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<SensorRttTelemetryTask*>(ctx)->run();
}

void SensorRttTelemetryTask::ApplyCommand(
    const app::telemetry::SensorRttTelemetryCommand& cmd) noexcept {
  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kOff) {
    enabled_ = false;
    sensor_id_ = 0;
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kObserve) {
    const domain::sensors::SensorState* sensor = registry_.FindById(cmd.sensor_id);
    if (sensor == nullptr) {
      enabled_ = false;
      sensor_id_ = 0;
      return;
    }

    enabled_ = true;
    sensor_id_ = cmd.sensor_id;
    mode_ = cmd.mode;
    return;
  }

  if (cmd.kind == app::telemetry::SensorRttTelemetryCommandKind::kSetPeriod) {
    period_ms_ = ClampPeriodMs(cmd.period_ms);
    return;
  }
}

void SensorRttTelemetryTask::run() noexcept {
  enabled_ = false;
  sensor_id_ = 0;
  mode_ = domain::sensors::SensorRttMode::kPosition;
  period_ms_ = app::config::RTT_TELEMETRY_SENSOR_PERIOD_MS;

  for (;;) {
    if (!enabled_) {
      app::telemetry::SensorRttTelemetryCommand cmd{};
      if (control_queue_.Receive(cmd, os::kWaitForever)) {
        ApplyCommand(cmd);
      }
      continue;
    }

    const std::uint8_t id = sensor_id_;
    domain::sensors::SensorState* sensor = registry_.FindById(id);
    if (sensor == nullptr) {
      enabled_ = false;
      sensor_id_ = 0;
      continue;
    }

    app::telemetry::SensorRttTelemetryCommand cmd{};
    if (control_queue_.Receive(cmd, period_ms_)) {
      ApplyCommand(cmd);
      continue;
    }

    if (adc_state_.GetState() != app::analog::AcquisitionState::kEnabled) {
      continue;
    }

    switch (mode_) {
      case domain::sensors::SensorRttMode::kAdc:
        telemetry_sender_.Send(static_cast<float>(sensor->last_raw_value));
        break;
      case domain::sensors::SensorRttMode::kRawCurrent:
        telemetry_sender_.Send(sensor->last_current_ma * 1000.0f);
        break;
      case domain::sensors::SensorRttMode::kCurrent:
        telemetry_sender_.Send(sensor->last_filtered_current_ma * 1000.0f);
        break;
      case domain::sensors::SensorRttMode::kPosition:
        telemetry_sender_.Send(sensor->last_normalized_position * 1000.0f);
        break;
    }
  }
}

bool SensorRttTelemetryTask::start() noexcept {
  return os::Task::create("SensorRtt", SensorRttTelemetryTask::entry, this,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_STACK_BYTES,
                          app::config::SENSOR_RTT_TELEMETRY_TASK_PRIORITY);
}

}  // namespace app::Tasks
