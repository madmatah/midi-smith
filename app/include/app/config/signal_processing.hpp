#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "app/config/sensor_linearization.hpp"
#include "app/telemetry/sensor_rtt_stream_tap.hpp"
#include "domain/dsp/converters/tia_current_converter.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/dsp/filters/biquad.hpp"
#include "domain/dsp/filters/identity_filter.hpp"
#include "domain/dsp/logic/input_gate.hpp"
#include "domain/dsp/math/sliding_linear_regression.hpp"
#include "domain/sensors/capture_sensor_state.hpp"
#include "domain/sensors/linearization/sensor_linear_processor.hpp"
#include "domain/sensors/sensor_member_reader.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace app::config {

constexpr std::int32_t ADC_REFERENCE_VOLTAGE_MILLI_VOLTS = 2048;
constexpr std::int32_t ADC_RESOLUTION_BITS = 16;
constexpr std::int32_t TIA_FEEDBACK_RESISTOR_OHMS = 1800;

constexpr float HAMMER_POSITION_DAMPER = 0.45f;
constexpr float HAMMER_POSITION_CATCH = 0.385f;
constexpr float HAMMER_POSITION_LETOFF = 0.0292f;

constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr float SIGNAL_NOTCH_CUTOFF_HZ = 293.9f;
constexpr float SIGNAL_NOTCH_Q_FACTOR = 100.0f;

constexpr std::int32_t SIGNAL_LOW_PASS_CUTOFF_HZ = 300;
constexpr float SIGNAL_LOW_PASS_Q_FACTOR = 0.707f;

constexpr std::size_t SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE = 5u;


namespace signal_processing_detail {

template <typename... StageTs>
using Workflow = domain::dsp::engine::Workflow<StageTs...>;
template <typename ContentT>
using Tap = domain::dsp::engine::Tap<ContentT>;


using LowPassFilter = domain::dsp::filters::Biquad<domain::dsp::filters::LowPassStrategy<
    ANALOG_ACQUISITION_CHANNEL_RATE_HZ, SIGNAL_LOW_PASS_CUTOFF_HZ, SIGNAL_LOW_PASS_Q_FACTOR>>;

using NotchFilter = domain::dsp::filters::Biquad<domain::dsp::filters::NotchStrategy<
    ANALOG_ACQUISITION_CHANNEL_RATE_HZ, SIGNAL_NOTCH_CUTOFF_HZ, SIGNAL_NOTCH_Q_FACTOR>>;

using FilteringEnabledPipeline = Workflow<NotchFilter, LowPassFilter>;


using FilteringDisabledPipeline = Workflow<domain::dsp::filters::IdentityFilter>;

using FilteringStage = std::conditional_t<SIGNAL_FILTERING_ENABLED, FilteringEnabledPipeline,
                                          FilteringDisabledPipeline>;


using TiaCurrentConverter =
    domain::dsp::converters::TiaCurrentConverter<ADC_REFERENCE_VOLTAGE_MILLI_VOLTS,
                                                 ADC_RESOLUTION_BITS, TIA_FEEDBACK_RESISTOR_OHMS>;


using AnalogSensorLinearizer =
    domain::sensors::linearization::SensorLinearProcessor<kSensorLookupTableSize>;

using HammerSpeedEstimator =
    domain::dsp::math::SlidingLinearRegression<SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE>;


using SensorState = domain::sensors::SensorState;

template <auto SensorMemberPtr>
using CaptureState = domain::sensors::CaptureSensorState<SensorMemberPtr>;
template <auto SensorMemberPtr>
using SensorMemberReader = domain::sensors::SensorMemberReader<SensorMemberPtr>;


using HammerSpeedGate =
    domain::dsp::logic::InputGate<HAMMER_POSITION_DAMPER,
                                  SensorMemberReader<&SensorState::last_normalized_position>{}>;
using HammerSpeedStage = Workflow<HammerSpeedEstimator, HammerSpeedGate,
                                  CaptureState<&SensorState::last_speed_units_per_ms>>;


// clang-format off
using SignalProcessingWorkflow = Workflow<
    FilteringStage,
    CaptureState<&SensorState::last_filtered_adc_value>,
    TiaCurrentConverter,
    CaptureState<&SensorState::last_current_ma>,
    AnalogSensorLinearizer,
    CaptureState<&SensorState::last_normalized_position>,
    Tap<HammerSpeedStage>,
    app::telemetry::SensorRttStreamTap
>;
// clang-format on

}  // namespace signal_processing_detail


constexpr std::size_t kAnalogSensorProcessorLinearizerStageIndex = 4u;
constexpr std::size_t kAnalogSensorProcessorRttTapStageIndex = 7u;
using AnalogSensorProcessor = signal_processing_detail::SignalProcessingWorkflow;

}  // namespace app::config
