#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "app/config/analog_acquisition.hpp"
#include "app/config/sensor_linearization.hpp"
#include "app/telemetry/sensor_rtt_stream_tap.hpp"
#include "domain/dsp/converters/linear_scaler.hpp"
#include "domain/dsp/converters/tia_current_converter.hpp"
#include "domain/dsp/engine/temporal_continuity_guard.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/dsp/filters/biquad.hpp"
#include "domain/dsp/filters/constant_filter.hpp"
#include "domain/dsp/filters/ema_filter.hpp"
#include "domain/dsp/filters/identity_filter.hpp"
#include "domain/dsp/filters/simple_moving_average.hpp"
#include "domain/dsp/logic/gate_open.hpp"
#include "domain/dsp/logic/input_gate.hpp"
#include "domain/dsp/logic/switch.hpp"
#include "domain/dsp/math/sliding_linear_regression.hpp"
#include "domain/music/piano/midi_velocity_engine.hpp"
#include "domain/music/piano/note_release_detector_stage.hpp"
#include "domain/sensors/capture_sensor_state.hpp"
#include "domain/sensors/linearization/sensor_linear_processor.hpp"
#include "domain/sensors/sensor_member_reader.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace app::config {

constexpr std::int32_t ADC_REFERENCE_VOLTAGE_MILLI_VOLTS = 2048;
constexpr std::int32_t ADC_RESOLUTION_BITS = 16;
constexpr std::int32_t TIA_FEEDBACK_RESISTOR_OHMS = 1800;

constexpr float HAMMER_POSITION_STRIKE = 0.005f;
constexpr float HAMMER_POSITION_LETOFF = 0.054f;
constexpr float HAMMER_POSITION_DROP = 0.078f;
constexpr float HAMMER_POSITION_CATCH = 0.5f;
constexpr float HAMMER_POSITION_REARM = 0.1;
constexpr float HAMMER_POSITION_DAMPER = 0.55f;

constexpr float R_hammer_mm = 126.0f;
constexpr float R_sensor_mm = 17.5f;
constexpr float L_ratio = R_hammer_mm / R_sensor_mm;
constexpr float Delta_d_mm =
    kDefaultSensorCalibration.rest_distance_mm - kDefaultSensorCalibration.strike_distance_mm;

constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr float SIGNAL_NOTCH_CUTOFF_HZ = 293.9f;
constexpr float SIGNAL_NOTCH_Q_FACTOR = 100.0f;

constexpr std::int32_t SIGNAL_EMA_FILTER_RATIO_NUMERATOR = 1;
constexpr std::int32_t SIGNAL_EMA_FILTER_RATIO_DENOMINATOR = 2;

constexpr std::size_t SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE = 5u;
constexpr std::uint32_t SIGNAL_HAMMER_SPEED_SMA_WINDOW_SIZE = 16u;

constexpr std::uint32_t SIGNAL_TEMPORAL_CONTINUITY_GAP_FACTOR = 5u;


namespace signal_processing_detail {

template <typename... StageTs>
using Workflow = domain::dsp::engine::Workflow<StageTs...>;
template <typename ContentT>
using Tap = domain::dsp::engine::Tap<ContentT>;
template <typename StageT, std::uint32_t kGapFactor>
using TemporalContinuityGuard = domain::dsp::engine::TemporalContinuityGuard<StageT, kGapFactor>;
template <typename PredicateT, typename TrueStageT, typename FalseStageT>
using Switch = domain::dsp::logic::Switch<PredicateT, TrueStageT, FalseStageT>;
template <float kValue>
using ConstantFilter = domain::dsp::filters::ConstantFilter<kValue>;
template <float kScale>
using LinearScaler = domain::dsp::converters::LinearScaler<kScale>;


using FastEmaFilter = domain::dsp::filters::EmaFilterRatio<SIGNAL_EMA_FILTER_RATIO_NUMERATOR,
                                                           SIGNAL_EMA_FILTER_RATIO_DENOMINATOR>;
using FilteringEnabledPipeline = Workflow<FastEmaFilter>;


using FilteringDisabledPipeline = Workflow<domain::dsp::filters::IdentityFilter>;

using FilteringStage = std::conditional_t<SIGNAL_FILTERING_ENABLED, FilteringEnabledPipeline,
                                          FilteringDisabledPipeline>;


using TiaCurrentConverter =
    domain::dsp::converters::TiaCurrentConverter<ADC_REFERENCE_VOLTAGE_MILLI_VOLTS,
                                                 ADC_RESOLUTION_BITS, TIA_FEEDBACK_RESISTOR_OHMS>;


using AnalogSensorLinearizer =
    domain::sensors::linearization::SensorLinearProcessor<kSensorLookupTableSize>;

using ShankSpeedEstimator =
    Workflow<domain::dsp::math::SlidingLinearRegression<SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE>,
             domain::dsp::filters::SimpleMovingAverage<SIGNAL_HAMMER_SPEED_SMA_WINDOW_SIZE>>;
using GuardedShankSpeedEstimator =
    TemporalContinuityGuard<ShankSpeedEstimator, SIGNAL_TEMPORAL_CONTINUITY_GAP_FACTOR>;


using SensorState = domain::sensors::SensorState;

template <auto SensorMemberPtr>
using CaptureState = domain::sensors::CaptureSensorState<SensorMemberPtr>;
template <auto SensorMemberPtr>
using SensorMemberReader = domain::sensors::SensorMemberReader<SensorMemberPtr>;


using ShankPositionReader = SensorMemberReader<&SensorState::last_shank_position_norm>;
using IsShankInActiveZone =
    domain::dsp::logic::GateOpen<HAMMER_POSITION_DAMPER, ShankPositionReader{}>;

using SmartShankSlopeEstimator =
    Switch<IsShankInActiveZone, GuardedShankSpeedEstimator, ConstantFilter<0.0f>>;

constexpr float kTicksPerMillisecond = static_cast<float>(ANALOG_TICKS_PER_SECOND) / 1000.0f;

using PhysicalPositionStage = LinearScaler<Delta_d_mm>;
using TicksToMsNormalizer = LinearScaler<kTicksPerMillisecond>;
using ShankToHammerKinematicsStage = LinearScaler<L_ratio>;

using PhysicalVelocityPipeline =
    Workflow<PhysicalPositionStage, CaptureState<&SensorState::last_shank_position_mm>,
             SmartShankSlopeEstimator, TicksToMsNormalizer,
             CaptureState<&SensorState::last_shank_speed_m_per_s>, ShankToHammerKinematicsStage,
             CaptureState<&SensorState::last_hammer_speed_m_per_s>>;

using MidiVelocityEngineStage =
    domain::music::piano::MidiVelocityEngine<HAMMER_POSITION_DAMPER, HAMMER_POSITION_LETOFF,
                                             HAMMER_POSITION_STRIKE, HAMMER_POSITION_DROP,
                                             HAMMER_POSITION_REARM>;

using NoteReleaseDetectorStage =
    domain::music::piano::NoteReleaseDetectorStage<HAMMER_POSITION_DAMPER>;


// clang-format off
using SignalProcessingWorkflow = Workflow<
    FilteringStage,
    CaptureState<&SensorState::last_filtered_adc_value>,
    TiaCurrentConverter,
    CaptureState<&SensorState::last_current_ma>,
    AnalogSensorLinearizer,
    CaptureState<&SensorState::last_shank_position_norm>,
    Tap<PhysicalVelocityPipeline>,
    Tap<MidiVelocityEngineStage>,
    Tap<NoteReleaseDetectorStage>,
    app::telemetry::SensorRttStreamTap
>;
// clang-format on

}  // namespace signal_processing_detail


constexpr std::size_t kAnalogSensorProcessorLinearizerStageIndex = 4u;
constexpr std::size_t kAnalogSensorProcessorMidiVelocityTapStageIndex = 7u;
constexpr std::size_t kAnalogSensorProcessorNoteReleaseTapStageIndex = 8u;
constexpr std::size_t kAnalogSensorProcessorRttTapStageIndex = 9u;
using AnalogSensorProcessor = signal_processing_detail::SignalProcessingWorkflow;

}  // namespace app::config
