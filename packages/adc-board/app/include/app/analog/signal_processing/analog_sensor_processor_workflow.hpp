#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "app/config/signal_processing.hpp"
#include "app/telemetry/sensor_rtt_stream_tap.hpp"
#include "domain/dsp/converters/linear_scaler.hpp"
#include "domain/dsp/converters/tia_current_converter.hpp"
#include "domain/dsp/engine/tap.hpp"
#include "domain/dsp/engine/temporal_continuity_guard.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/dsp/filters/constant_filter.hpp"
#include "domain/dsp/filters/identity_filter.hpp"
#include "domain/dsp/filters/simple_moving_average.hpp"
#include "domain/dsp/logic/and.hpp"
#include "domain/dsp/logic/gate_open.hpp"
#include "domain/dsp/logic/is_true.hpp"
#include "domain/dsp/logic/switch.hpp"
#include "domain/dsp/math/central_difference.hpp"
#include "domain/dsp/math/sliding_linear_regression.hpp"
#include "domain/music/piano/key_action_requirements.hpp"
#include "domain/music/piano/midi_velocity_engine.hpp"
#include "domain/music/piano/note_release_detector_stage.hpp"
#include "domain/music/piano/velocity/goebl_logarithmic_velocity_mapper.hpp"
#include "domain/music/piano/velocity/logarithmic_velocity_mapper.hpp"
#include "domain/sensors/capture_sensor_state.hpp"
#include "domain/sensors/linearization/sensor_linear_processor.hpp"
#include "domain/sensors/sensor_member_reader.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace app::analog::signal_processing::workflow {

// -- Template shorthand aliases ---------------------------------------------------

template <typename... StageTs>
using StageWorkflow = domain::dsp::engine::Workflow<StageTs...>;
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
template <std::uint32_t kWindowSize>
using SimpleMovingAverage = domain::dsp::filters::SimpleMovingAverage<kWindowSize>;
template <std::uint32_t kWindowSize>
using SlidingLinearRegression = domain::dsp::math::SlidingLinearRegression<kWindowSize>;
using CentralDifference = domain::dsp::math::CentralDifference;
template <auto SensorMemberPtr>
using CaptureState = domain::sensors::CaptureSensorState<SensorMemberPtr>;
template <auto SensorMemberPtr>
using SensorMemberReader = domain::sensors::SensorMemberReader<SensorMemberPtr>;
using SensorState = domain::sensors::SensorState;
template <typename... PredicateTs>
using And = domain::dsp::logic::And<PredicateTs...>;
template <auto ValueProvider>
using IsTrue = domain::dsp::logic::IsTrue<ValueProvider>;
using GoeblLogarithmicVelocityMapper =
    domain::music::piano::velocity::GoeblLogarithmicVelocityMapper;
template <float kMaximumSpeedMPerS, float kShapeFactor>
using LogarithmicVelocityMapper =
    domain::music::piano::velocity::LogarithmicVelocityMapper<kMaximumSpeedMPerS, kShapeFactor>;

// =============================================================================
// Preprocessing Trunk
//   ADC -> [Filter] -> [TIA Current Converter] -> [Linearizer]
// =============================================================================

using FilteringEnabledPipeline =
    StageWorkflow<SimpleMovingAverage<config::ADC_OUTPUT_SMA_WINDOW_SIZE>>;
using FilteringDisabledPipeline = StageWorkflow<domain::dsp::filters::IdentityFilter>;
using FilteringStage = std::conditional_t<config::SIGNAL_FILTERING_ENABLED,
                                          FilteringEnabledPipeline, FilteringDisabledPipeline>;

using TiaCurrentConverter =
    domain::dsp::converters::TiaCurrentConverter<config::ADC_REFERENCE_VOLTAGE_MILLI_VOLTS,
                                                 config::ADC_RESOLUTION_BITS,
                                                 config::TIA_FEEDBACK_RESISTOR_OHMS>;

using LinearizerStage =
    domain::sensors::linearization::SensorLinearProcessor<config::kSensorLookupTableSize>;

// =============================================================================
// TAP 1 - Physical Velocity Pipeline
//   Position -> OLS -> SMA -> Shank Speed -> Hammer Kinematics
// =============================================================================

using ShankPositionReader = SensorMemberReader<&SensorState::last_shank_position_norm>;

using GuardedShankSpeedEstimator = TemporalContinuityGuard<
    StageWorkflow<SlidingLinearRegression<config::SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE>,
                  SimpleMovingAverage<config::SIGNAL_HAMMER_SPEED_SMA_WINDOW_SIZE>>,
    config::SIGNAL_TEMPORAL_CONTINUITY_GAP_FACTOR>;

using IsShankInActiveZone =
    domain::dsp::logic::GateOpen<config::HAMMER_POSITION_DAMPER, ShankPositionReader{}>;

using SmartShankSlopeEstimator =
    Switch<IsShankInActiveZone, GuardedShankSpeedEstimator, ConstantFilter<0.0f>>;

using PhysicalPositionStage = LinearScaler<config::Delta_d_mm>;
using TicksToMsNormalizer = LinearScaler<config::kTicksPerMillisecond>;
using ShankToHammerKinematicsStage = LinearScaler<config::L_ratio>;

using PhysicalVelocityPipeline =
    StageWorkflow<PhysicalPositionStage, CaptureState<&SensorState::last_shank_position_mm>,
                  SmartShankSlopeEstimator, TicksToMsNormalizer,
                  CaptureState<&SensorState::last_shank_speed_m_per_s>,
                  ShankToHammerKinematicsStage,
                  CaptureState<&SensorState::last_hammer_speed_m_per_s>>;
using PhysicalVelocityPipelineTap = Tap<PhysicalVelocityPipeline>;


// =============================================================================
// TAP 2 - MIDI Velocity Engine (Note-On)
//   Hammer Speed -> Goebl Velocity Mapper -> Note-On Detection
// =============================================================================

using NoteOnVelocityMapper = GoeblLogarithmicVelocityMapper;
using NoteOnStage = domain::music::piano::MidiVelocityEngine<
    NoteOnVelocityMapper, config::HAMMER_POSITION_DAMPER, config::HAMMER_POSITION_LETOFF,
    config::HAMMER_POSITION_STRIKE, config::HAMMER_POSITION_DROP, config::HAMMER_POSITION_REARM>;

using NoteOnTapStage = Tap<NoteOnStage>;


// =============================================================================
// TAP 3 - Damper Release Pipeline
//   Position -> Heavy SMA -> Central Diff -> Falling Speed
// =============================================================================

using ShankPositionSmoothedReader =
    SensorMemberReader<&SensorState::last_shank_position_smoothed_norm>;
using NoteOnReader = SensorMemberReader<&SensorState::is_note_on>;

using IsNoteOn = IsTrue<NoteOnReader{}>;
using IsSmoothedShankPositionInActiveZone =
    domain::dsp::logic::GateOpen<config::HAMMER_POSITION_DAMPER, ShankPositionSmoothedReader{}>;
using IsDamperFalling = And<IsSmoothedShankPositionInActiveZone, IsNoteOn>;

using GuardedFallingShankSpeedEstimator = TemporalContinuityGuard<
    StageWorkflow<CentralDifference,
                  SimpleMovingAverage<config::SIGNAL_FALLING_SHANK_SPEED_SMA_WINDOW_SIZE>>,
    config::SIGNAL_TEMPORAL_CONTINUITY_GAP_FACTOR>;

using SmartFallingShankSpeedEstimator =
    Switch<IsDamperFalling, GuardedFallingShankSpeedEstimator, ConstantFilter<0.0f>>;

using SmoothedPositionFilter =
    SimpleMovingAverage<config::SIGNAL_SMOOTHED_POSITION_SMA_WINDOW_SIZE>;

using DamperReleasePhysicalStage =
    StageWorkflow<SmoothedPositionFilter,
                  CaptureState<&SensorState::last_shank_position_smoothed_norm>,
                  PhysicalPositionStage, SmartFallingShankSpeedEstimator, TicksToMsNormalizer,
                  LinearScaler<config::SIGNAL_SMOOTHED_POSITION_SCALE_FACTOR>,
                  CaptureState<&SensorState::last_shank_falling_speed_m_per_s>>;
using DamperReleasePhysicalStageStage = Tap<DamperReleasePhysicalStage>;

// =============================================================================
// TAP 4 - Note Release Detector (Note-Off)
//   Falling Speed -> Logarithmic Mapper -> Note-Off Detection
// =============================================================================

using NoteOffVelocityMapper = LogarithmicVelocityMapper<config::NOTE_OFF_VELOCITY_MAX_M_PER_S,
                                                        config::NOTE_OFF_VELOCITY_SHAPE_FACTOR>;
using NoteOffStage = domain::music::piano::NoteReleaseDetectorStage<NoteOffVelocityMapper,
                                                                    config::HAMMER_POSITION_DAMPER>;
using NoteOffTapStage = Tap<NoteOffStage>;


// =============================================================================
// TAP 5 - Telemetry
//   Sensor State -> Telemetry
// =============================================================================

using TelemetryTapStage = app::telemetry::SensorRttStreamTap;

// =============================================================================
// Final Assembly
//   ADC -> Filter -> TIA -> Linearizer -> [TAP1..TAP4] -> Telemetry
// =============================================================================

// clang-format off
using Workflow = StageWorkflow<
    FilteringStage,
    CaptureState<&SensorState::last_filtered_adc_value>,
    TiaCurrentConverter,
    CaptureState<&SensorState::last_current_ma>,
    LinearizerStage,
    CaptureState<&SensorState::last_shank_position_norm>,
    PhysicalVelocityPipelineTap,
    NoteOnTapStage,
    DamperReleasePhysicalStageStage,
    NoteOffTapStage,
    TelemetryTapStage
>;
// clang-format on


using ProcessorWorkflow = Workflow;
using LinearizerConfiguration = typename LinearizerStage::Configuration;
using KeyActionHandler = domain::music::piano::KeyActionRequirements;

struct StageAccess {
  static constexpr std::size_t kLinearizerStageIndex =
      Workflow::template StageIndexOf<LinearizerStage>;
  static constexpr std::size_t kTelemetryTapStageIndex =
      Workflow::template StageIndexOf<TelemetryTapStage>;
  static constexpr std::size_t kNoteOnTapStageIndex =
      Workflow::template StageIndexOf<NoteOnTapStage>;
  static constexpr std::size_t kNoteOffTapStageIndex =
      Workflow::template StageIndexOf<NoteOffTapStage>;

  static LinearizerStage& Linearizer(Workflow& workflow) noexcept {
    return workflow.template Stage<kLinearizerStageIndex>();
  }

  static TelemetryTapStage& TelemetryTap(Workflow& workflow) noexcept {
    return workflow.template Stage<kTelemetryTapStageIndex>();
  }

  static NoteOnTapStage& NoteOnTap(Workflow& workflow) noexcept {
    return workflow.template Stage<kNoteOnTapStageIndex>();
  }

  static NoteOffTapStage& NoteOffTap(Workflow& workflow) noexcept {
    return workflow.template Stage<kNoteOffTapStageIndex>();
  }
};

struct ControlSurface {
  static void SetLinearizerConfiguration(ProcessorWorkflow& workflow,
                                         const LinearizerConfiguration* configuration) noexcept {
    StageAccess::Linearizer(workflow).ApplyConfiguration(configuration);
  }

  static void SetTelemetryCapture(ProcessorWorkflow& workflow,
                                  app::telemetry::SensorRttStreamCapture* capture) noexcept {
    StageAccess::TelemetryTap(workflow).SetCapture(capture);
  }

  static void SetNoteOnKeyActionHandler(ProcessorWorkflow& workflow,
                                        KeyActionHandler* handler) noexcept {
    StageAccess::NoteOnTap(workflow).Content().SetKeyActionHandler(handler);
  }

  static void SetNoteOffKeyActionHandler(ProcessorWorkflow& workflow,
                                         KeyActionHandler* handler) noexcept {
    StageAccess::NoteOffTap(workflow).Content().SetKeyActionHandler(handler);
  }
};

}  // namespace app::analog::signal_processing::workflow
