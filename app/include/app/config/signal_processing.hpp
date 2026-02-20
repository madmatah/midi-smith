#pragma once

#include <cstddef>
#include <cstdint>

#include "app/config/analog_acquisition.hpp"
#include "app/config/sensor_linearization.hpp"

namespace app::config {

// ── Hardware: ADC + TIA ──────────────────────────────────────────────

constexpr std::int32_t ADC_REFERENCE_VOLTAGE_MILLI_VOLTS = 2048;
constexpr std::int32_t ADC_RESOLUTION_BITS = 16;
constexpr std::int32_t TIA_FEEDBACK_RESISTOR_OHMS = 1800;

// ── Piano mechanics: hammer geometry & key positions ─────────────────

constexpr float HAMMER_POSITION_STRIKE = 0.005f;
constexpr float HAMMER_POSITION_LETOFF = 0.054f;
constexpr float HAMMER_POSITION_DROP = 0.078f;
constexpr float HAMMER_POSITION_CATCH = 0.5f;
constexpr float HAMMER_POSITION_REARM = 0.1f;
constexpr float HAMMER_POSITION_DAMPER = 0.55f;

constexpr float R_hammer_mm = 126.0f;
constexpr float R_sensor_mm = 17.5f;
constexpr float L_ratio = R_hammer_mm / R_sensor_mm;
constexpr float Delta_d_mm =
    kDefaultSensorCalibration.rest_distance_mm - kDefaultSensorCalibration.strike_distance_mm;

// ── Note-off velocity mapping ────────────────────────────────────────

constexpr float NOTE_OFF_VELOCITY_MAX_M_PER_S = 0.08f;
constexpr float NOTE_OFF_VELOCITY_SHAPE_FACTOR = 1.8f;

// ── DSP tuning: filter windows & estimator parameters ────────────────

constexpr bool SIGNAL_FILTERING_ENABLED = true;

constexpr std::uint32_t ADC_OUTPUT_SMA_WINDOW_SIZE = 10u;

constexpr std::size_t SIGNAL_HAMMER_SPEED_ESTIMATOR_WINDOW_SIZE = 5u;
constexpr std::uint32_t SIGNAL_HAMMER_SPEED_SMA_WINDOW_SIZE = 16u;

constexpr std::uint32_t SIGNAL_FALLING_SHANK_SPEED_SMA_WINDOW_SIZE = 30u;
constexpr std::uint32_t SIGNAL_SMOOTHED_POSITION_SMA_WINDOW_SIZE = 100u;
constexpr float SIGNAL_SMOOTHED_POSITION_SCALE_FACTOR = 1.15f;

constexpr std::uint32_t SIGNAL_TEMPORAL_CONTINUITY_GAP_FACTOR = 5u;

// ── Derived constants ────────────────────────────────────────────────

constexpr float kTicksPerMillisecond = static_cast<float>(ANALOG_TICKS_PER_SECOND) / 1000.0f;

}  // namespace app::config
