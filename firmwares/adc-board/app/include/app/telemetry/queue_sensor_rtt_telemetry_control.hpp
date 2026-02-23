#pragma once

#include <cstdint>

#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_command.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "os/queue.hpp"

namespace midismith::adc_board::app::telemetry {

class QueueSensorRttTelemetryControl final : public SensorRttTelemetryControlRequirements {
 public:
  QueueSensorRttTelemetryControl(::midismith::os::Queue<SensorRttTelemetryCommand, 4>& queue,
                                 SensorRttStreamCapture& capture) noexcept
      : queue_(queue), capture_(capture) {}

  bool RequestOff() noexcept override {
    SensorRttTelemetryCommand cmd{};
    cmd.kind = SensorRttTelemetryCommandKind::kOff;
    return queue_.Send(cmd, ::midismith::os::kNoWait);
  }

  bool RequestObserve(std::uint8_t sensor_id) noexcept override {
    SensorRttTelemetryCommand cmd{};
    cmd.kind = SensorRttTelemetryCommandKind::kObserve;
    cmd.sensor_id = sensor_id;
    return queue_.Send(cmd, ::midismith::os::kNoWait);
  }

  bool RequestSetOutputHz(std::uint32_t output_hz) noexcept override {
    SensorRttTelemetryCommand cmd{};
    cmd.kind = SensorRttTelemetryCommandKind::kSetOutputHz;
    cmd.output_hz = output_hz;
    return queue_.Send(cmd, ::midismith::os::kNoWait);
  }

  SensorRttTelemetryStatus GetStatus() const noexcept override {
    SensorRttTelemetryStatus s{};
    const auto st = capture_.GetStatus();
    s.enabled = st.enabled;
    s.sensor_id = st.sensor_id;
    s.output_hz = st.output_hz;
    s.dropped_frames = st.dropped_frames;
    s.backlog_frames = st.backlog_frames;
    return s;
  }

 private:
  ::midismith::os::Queue<SensorRttTelemetryCommand, 4>& queue_;
  SensorRttStreamCapture& capture_;
};

}  // namespace midismith::adc_board::app::telemetry
