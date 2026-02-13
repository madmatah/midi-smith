#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "app/config/sensor_linearization.hpp"
#include "app/telemetry/sensor_rtt_stream_tap.hpp"
#include "domain/dsp/converters/tia_current_converter.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/dsp/filters/biquad.hpp"
#include "domain/dsp/filters/ema_filter.hpp"
#include "domain/dsp/filters/identity_filter.hpp"
#include "domain/sensors/capture_sensor_state.hpp"
#include "domain/sensors/linearization/sensor_linear_processor.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace app::config {

constexpr std::int32_t ADC_REFERENCE_VOLTAGE_MILLI_VOLTS = 2048;
constexpr std::int32_t ADC_RESOLUTION_BITS = 16;
constexpr std::int32_t TIA_FEEDBACK_RESISTOR_OHMS = 1800;


constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr std::int32_t SIGNAL_EMA_ALPHA_NUMERATOR = 1;
constexpr std::int32_t SIGNAL_EMA_ALPHA_DENOMINATOR = 8;

constexpr std::int32_t SIGNAL_LOW_PASS_CUTOFF_HZ = 400;
constexpr float SIGNAL_LOW_PASS_Q_FACTOR = 0.707f;

// Decimation factor is applied on segments of the pipeline to reduce processing frequency.
// Set to 1 to disable decimation.
constexpr std::uint8_t SIGNAL_DECIMATION_FACTOR = 1;

namespace signal_processing_detail {

using LowPassFilter = domain::dsp::filters::Biquad<domain::dsp::filters::LowPassStrategy<
    ANALOG_ACQUISITION_CHANNEL_RATE_HZ, SIGNAL_LOW_PASS_CUTOFF_HZ, SIGNAL_LOW_PASS_Q_FACTOR>>;
using EmaFilter =
    domain::dsp::filters::EmaFilterRatio<SIGNAL_EMA_ALPHA_NUMERATOR, SIGNAL_EMA_ALPHA_DENOMINATOR>;

using NotchFilter1 = domain::dsp::filters::Biquad<
    domain::dsp::filters::NotchStrategy<3500u, 97u, 25.0f>>;  // remove peak at 97.4 Hz
using NotchFilter2 = domain::dsp::filters::Biquad<
    domain::dsp::filters::NotchStrategy<3500u, 294u, 25.0f>>;  // remove peak at 293.9 Hz

using FilteringEnabledPipeline =
    domain::dsp::engine::Workflow<NotchFilter1, NotchFilter2, LowPassFilter>;


using FilteringDisabledPipeline =
    domain::dsp::engine::Workflow<domain::dsp::filters::IdentityFilter>;

using FilteringStage = std::conditional_t<SIGNAL_FILTERING_ENABLED, FilteringEnabledPipeline,
                                          FilteringDisabledPipeline>;


using TiaCurrentConverter =
    domain::dsp::converters::TiaCurrentConverter<ADC_REFERENCE_VOLTAGE_MILLI_VOLTS,
                                                 ADC_RESOLUTION_BITS, TIA_FEEDBACK_RESISTOR_OHMS>;


using AnalogSensorLinearizer =
    domain::sensors::linearization::SensorLinearProcessor<kSensorLookupTableSize>;


using SensorState = domain::sensors::SensorState;

template <auto SensorMemberPtr>
using CaptureState = domain::sensors::CaptureSensorState<SensorMemberPtr>;


// clang-format off
using SignalProcessingWorkflow = domain::dsp::engine::Workflow<
    FilteringStage,
    CaptureState<&SensorState::last_filtered_adc_value>,
    TiaCurrentConverter,
    CaptureState<&SensorState::last_current_ma>,
    AnalogSensorLinearizer,
    CaptureState<&SensorState::last_normalized_position>,
    app::telemetry::SensorRttStreamTap
>;
// clang-format on

}  // namespace signal_processing_detail


constexpr std::size_t kAnalogSensorProcessorLinearizerStageIndex = 4u;
constexpr std::size_t kAnalogSensorProcessorRttTapStageIndex = 6u;
using AnalogSensorProcessor = signal_processing_detail::SignalProcessingWorkflow;

}  // namespace app::config
