#pragma once

#include <cstdint>

#include "app/analog/acquisition_command.hpp"
#include "app/analog/acquisition_state.hpp"
#include "app/analog/adc_rank_mapped_frame_decoder.hpp"
#include "app/analog/signal_context.hpp"
#include "app/analog/signal_processing/analog_sensor_processor.hpp"
#include "app/config/sensors.hpp"
#include "bsp/adc/adc_dma.hpp"
#include "bsp/gpio_requirements.hpp"
#include "bsp/time/timestamp_counter_requirements.hpp"
#include "domain/calibration/calibration_data_collector.hpp"
#include "domain/sensors/processed_sensor_group.hpp"
#include "os/queue.hpp"
#include "os/queue_requirements.hpp"

namespace midismith::adc_board::app::analog {
class AcquisitionSequencer;
}  // namespace midismith::adc_board::app::analog

namespace midismith::adc_board::app::tasks {

class AnalogAcquisitionTask {
 public:
  using Processor = midismith::adc_board::app::analog::signal_processing::AnalogSensorProcessor;
  using ProcessedSensorGroup = midismith::adc_board::domain::sensors::ProcessedSensorGroup<
      Processor, midismith::adc_board::app::analog::SignalContext>;
  using AdcFrameDescriptor = midismith::adc_board::bsp::adc::AdcFrameDescriptor;
  using AcquisitionCommand = midismith::adc_board::app::analog::AcquisitionCommand;
  using AcquisitionSequencer = midismith::adc_board::app::analog::AcquisitionSequencer;
  using AdcDma = midismith::adc_board::bsp::adc::AdcDma;
  using AdcRankMappedFrameDecoder = midismith::adc_board::app::analog::AdcRankMappedFrameDecoder;
  using AnalogAcquisitionState = midismith::adc_board::app::analog::AcquisitionState;
  using GpioRequirements = midismith::bsp::GpioRequirements;
  using TimestampCounterRequirements = midismith::bsp::time::TimestampCounterRequirements;
  using CalibrationDataCollector =
      midismith::adc_board::domain::calibration::CalibrationDataCollector<
          midismith::adc_board::app::config::sensors::kSensorCount>;
  using CalibrationArray = CalibrationDataCollector::CalibrationArray;
  using SensorRegistry = midismith::adc_board::domain::sensors::SensorRegistry;

  AnalogAcquisitionTask(
      midismith::os::Queue<AdcFrameDescriptor, 8>& queue,
      midismith::os::Queue<AcquisitionCommand, 4>& control_queue, GpioRequirements& tia_shutdown,
      AdcDma& adc_dma, TimestampCounterRequirements& timestamp_counter,
      volatile AnalogAcquisitionState& state, ProcessedSensorGroup& analog_group,
      midismith::os::QueueRequirements<CalibrationArray>& calibration_result_queue,
      SensorRegistry& sensor_registry) noexcept;

  bool start() noexcept;

 private:
  static void entry(void* ctx) noexcept;
  void run() noexcept;
  void ResetDecodingState() noexcept;
  void DrainFrameQueue() noexcept;
  void EnterDisabledState() noexcept;
  void HandleDisabledState(AcquisitionSequencer& sequencer) noexcept;
  void HandleEnabledState() noexcept;
  bool TryHandleCommandsWhileEnabled() noexcept;
  void HandleCalibrationStart() noexcept;
  void HandleRestPhaseComplete() noexcept;
  void HandleCollectCalibrationData() noexcept;
  void ProcessFrame(const AdcFrameDescriptor& desc) noexcept;
  void ProcessAdc1Frame(const AdcFrameDescriptor& desc) noexcept;
  void ProcessAdc2Frame(const AdcFrameDescriptor& desc) noexcept;
  void ProcessAdc3Frame(const AdcFrameDescriptor& desc) noexcept;

  midismith::os::Queue<AdcFrameDescriptor, 8>& queue_;
  midismith::os::Queue<AcquisitionCommand, 4>& control_queue_;
  GpioRequirements& tia_shutdown_;
  AdcDma& adc_dma_;
  TimestampCounterRequirements& timestamp_counter_;
  volatile AnalogAcquisitionState& state_;
  ProcessedSensorGroup& analog_group_;
  midismith::os::QueueRequirements<CalibrationArray>& calibration_result_queue_;
  CalibrationDataCollector collector_;

  AdcRankMappedFrameDecoder decoder_{};

  std::uint32_t last_adc1_sequence_id_ = 0;
  std::uint32_t last_adc2_sequence_id_ = 0;
  std::uint32_t last_adc3_sequence_id_ = 0;

  bool has_prev_adc1_timestamp_ = false;
  bool has_prev_adc2_timestamp_ = false;
  bool has_prev_adc3_timestamp_ = false;
  std::uint32_t prev_adc1_timestamp_ = 0;
  std::uint32_t prev_adc2_timestamp_ = 0;
  std::uint32_t prev_adc3_timestamp_ = 0;
};

}  // namespace midismith::adc_board::app::tasks
