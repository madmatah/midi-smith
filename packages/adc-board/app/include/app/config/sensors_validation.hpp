#pragma once

#include <cstddef>
#include <cstdint>

#include "app/config/sensors.hpp"

namespace app::config_sensors::validation {

template <std::size_t N>
constexpr bool HasZeroId(const std::uint8_t (&ids)[N]) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    if (ids[i] == 0u) {
      return true;
    }
  }
  return false;
}

template <std::size_t N>
constexpr bool AreUnique(const std::uint8_t (&ids)[N]) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    for (std::size_t j = i + 1u; j < N; ++j) {
      if (ids[i] == ids[j]) {
        return false;
      }
    }
  }
  return true;
}

template <std::size_t N, std::size_t SetN>
constexpr bool AllInSet(const std::uint8_t (&values)[N],
                        const std::uint8_t (&set_values)[SetN]) noexcept {
  for (std::size_t i = 0; i < N; ++i) {
    bool found = false;
    for (std::size_t j = 0; j < SetN; ++j) {
      if (values[i] == set_values[j]) {
        found = true;
        break;
      }
    }
    if (!found) {
      return false;
    }
  }
  return true;
}

template <std::size_t N>
constexpr bool IsContiguousFrom1(const std::uint8_t (&ids)[N]) noexcept {
  if (N == 0) {
    return false;
  }
  for (std::size_t i = 0; i < N; ++i) {
    const std::uint8_t expected = static_cast<std::uint8_t>(i + 1u);
    if (ids[i] != expected) {
      return false;
    }
  }
  return true;
}

}  // namespace app::config_sensors::validation

static_assert(!app::config_sensors::validation::HasZeroId(app::config_sensors::kSensorIds),
              "Sensor IDs must be non-zero");
static_assert(app::config_sensors::kSensorCount > 0u, "At least one sensor must be configured");
static_assert(app::config_sensors::validation::AreUnique(app::config_sensors::kSensorIds),
              "Sensor IDs must be unique");
static_assert(app::config_sensors::validation::IsContiguousFrom1(app::config_sensors::kSensorIds),
              "Sensor IDs must be contiguous from 1");
static_assert(app::config_sensors::validation::AreUnique(app::config_sensors::kAnalogRankSensorIds),
              "Analog rank mapping must not contain duplicates");
static_assert(app::config_sensors::validation::AllInSet(app::config_sensors::kAnalogRankSensorIds,
                                                        app::config_sensors::kSensorIds),
              "Analog rank mapping must reference valid IDs");
static_assert(app::config_sensors::validation::AllInSet(app::config_sensors::kSensorIds,
                                                        app::config_sensors::kAnalogRankSensorIds),
              "Analog rank mapping must cover all sensor IDs");
