#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "app/analog/acquisition_state_requirements.hpp"
#include "app/telemetry/sensor_rtt_stream_capture.hpp"
#include "app/telemetry/sensor_rtt_telemetry_command.hpp"
#include "app/telemetry/telemetry_sender_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "os/queue.hpp"

namespace midismith::adc_board::app::tasks {

class SensorRttTelemetryTask {
 public:
  SensorRttTelemetryTask(
      midismith::os::Queue<midismith::adc_board::app::telemetry::SensorRttTelemetryCommand, 4>&
          control_queue,
      midismith::adc_board::domain::sensors::SensorRegistry& registry,
      midismith::adc_board::app::analog::AcquisitionStateRequirements& adc_state,
      midismith::adc_board::app::telemetry::TelemetrySenderRequirements& telemetry_sender,
      midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;

  void ApplyCommand(
      const midismith::adc_board::app::telemetry::SensorRttTelemetryCommand& cmd) noexcept;
  void ApplyPendingCommands() noexcept;
  bool ReceiveControlCommand(std::uint32_t timeout_ms) noexcept;
  bool IsAnalogAcquisitionEnabled() const noexcept;
  bool TrySendSchemaFrameIfDue(std::uint32_t schema_interval_us,
                               std::span<std::uint8_t> schema_frame_bytes) noexcept;
  bool TrySendCapturedDataFrames(std::size_t max_frames_per_write) noexcept;

  midismith::os::Queue<midismith::adc_board::app::telemetry::SensorRttTelemetryCommand, 4>&
      control_queue_;
  midismith::adc_board::domain::sensors::SensorRegistry& registry_;
  midismith::adc_board::app::analog::AcquisitionStateRequirements& adc_state_;
  midismith::adc_board::app::telemetry::TelemetrySenderRequirements& telemetry_sender_;
  midismith::adc_board::app::telemetry::SensorRttStreamCapture& capture_;

  bool schema_sent_{false};
  std::uint32_t last_schema_timestamp_us_{0u};
  std::uint32_t last_data_timestamp_us_{0u};
  std::uint8_t active_sensor_id_{0u};
};

}  // namespace midismith::adc_board::app::tasks
