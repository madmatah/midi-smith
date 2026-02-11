#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>

#include "domain/sensors/linearization/sensor_calibration.hpp"
#include "domain/sensors/linearization/sensor_linear_processor_configuration.hpp"
#include "domain/sensors/linearization/sensor_lookup_table.hpp"
#include "domain/sensors/linearization/sensor_response_curve.hpp"

namespace domain::sensors::linearization {

enum class LookupTableGenerationStatus : std::uint8_t {
  kOk = 0,
  kInvalidCalibration = 1,
  kInvalidMasterSignature = 2,
};

template <std::size_t kLookupTableSize>
struct LookupTableGenerationResult {
  LookupTableGenerationStatus status = LookupTableGenerationStatus::kInvalidCalibration;
  SensorLinearProcessorConfiguration<kLookupTableSize> configuration{};
};

namespace lookup_table_generator_detail {

template <std::size_t kLookupTableSize>
inline void FillFallbackLookupTable(
    SensorLookupTable<kLookupTableSize>& lookup_table_storage) noexcept {
  for (std::size_t i = 0; i < kLookupTableSize; ++i) {
    const float u = static_cast<float>(i) / static_cast<float>(kLookupTableSize - 1u);
    lookup_table_storage[i] = 1.0f - u;  // 0=strike, 1=rest, and i=0 corresponds to rest
  }
}

template <std::size_t kLookupTableSize>
inline SensorLinearProcessorConfiguration<kLookupTableSize> MakeConfigurationFor(
    const SensorCalibration& calibration,
    const SensorLookupTable<kLookupTableSize>& lookup_table_storage) noexcept {
  SensorLinearProcessorConfiguration<kLookupTableSize> cfg{};
  cfg.lookup_table = &lookup_table_storage;

  const float current_span_ma = calibration.strike_current_ma - calibration.rest_current_ma;
  if (current_span_ma != 0.0f) {
    cfg.input_to_lut_index_scale = static_cast<float>(kLookupTableSize - 1u) / current_span_ma;
    cfg.input_to_lut_index_bias = -calibration.rest_current_ma * cfg.input_to_lut_index_scale;
  }
  return cfg;
}

}  // namespace lookup_table_generator_detail

class LookupTableGenerator {
 public:
  template <std::size_t kLookupTableSize>
  static LookupTableGenerationResult<kLookupTableSize> Generate(
      const SensorResponseCurve& master, const SensorCalibration& calibration,
      SensorLookupTable<kLookupTableSize>& lookup_table_storage) noexcept {
    static_assert(kLookupTableSize >= 2u, "kLookupTableSize must be >= 2");

    if (master.count() < 2u) {
      lookup_table_generator_detail::FillFallbackLookupTable(lookup_table_storage);
      return LookupTableGenerationResult<kLookupTableSize>{
          .status = LookupTableGenerationStatus::kInvalidMasterSignature,
          .configuration = lookup_table_generator_detail::MakeConfigurationFor(
              calibration, lookup_table_storage),
      };
    }

    const bool calibration_has_valid_distance_range =
        (calibration.rest_distance_mm > calibration.strike_distance_mm) &&
        (calibration.strike_distance_mm >= master.MinDistanceMm()) &&
        (calibration.rest_distance_mm <= master.MaxDistanceMm());
    const bool calibration_has_valid_current_range =
        (calibration.rest_current_ma != calibration.strike_current_ma);

    if (!calibration_has_valid_distance_range || !calibration_has_valid_current_range) {
      lookup_table_generator_detail::FillFallbackLookupTable(lookup_table_storage);
      return LookupTableGenerationResult<kLookupTableSize>{
          .status = LookupTableGenerationStatus::kInvalidCalibration,
          .configuration = lookup_table_generator_detail::MakeConfigurationFor(
              calibration, lookup_table_storage),
      };
    }

    const float rest_relative_current =
        master.RelativeCurrentAtDistanceMm(calibration.rest_distance_mm);
    const float strike_relative_current =
        master.RelativeCurrentAtDistanceMm(calibration.strike_distance_mm);

    const float current_span_ma = calibration.strike_current_ma - calibration.rest_current_ma;
    const float relative_current_span = strike_relative_current - rest_relative_current;
    const float distance_span_mm = calibration.rest_distance_mm - calibration.strike_distance_mm;

    for (std::size_t i = 0; i < kLookupTableSize; ++i) {
      const float u = static_cast<float>(i) / static_cast<float>(kLookupTableSize - 1u);
      const float current_ma = calibration.rest_current_ma + u * current_span_ma;

      const float projected_relative_current =
          rest_relative_current +
          (current_ma - calibration.rest_current_ma) * relative_current_span / current_span_ma;

      const float clamped_relative_current = std::clamp(projected_relative_current, 0.0f, 1.0f);
      const float distance_mm = master.DistanceMmAtRelativeCurrent(clamped_relative_current);

      const float position =
          (distance_mm - calibration.strike_distance_mm) / distance_span_mm;  // 0=strike, 1=rest

      lookup_table_storage[i] = std::clamp(position, 0.0f, 1.0f);
    }

    return LookupTableGenerationResult<kLookupTableSize>{
        .status = LookupTableGenerationStatus::kOk,
        .configuration =
            lookup_table_generator_detail::MakeConfigurationFor(calibration, lookup_table_storage),
    };
  }
};

}  // namespace domain::sensors::linearization
