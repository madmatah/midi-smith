#pragma once

#include <cstddef>
#include <cstdint>

#include "app/config/sensors.hpp"

namespace midismith::adc_board::app::config::sensors::validation {

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

}  // namespace midismith::adc_board::app::config::sensors::validation

static_assert(!midismith::adc_board::app::config::sensors::validation::HasZeroId(
                  midismith::adc_board::app::config::sensors::kSensorIds),
              "Sensor IDs must be non-zero");
static_assert(midismith::adc_board::app::config::sensors::kSensorCount > 0u,
              "At least one sensor must be configured");
static_assert(midismith::adc_board::app::config::sensors::validation::AreUnique(
                  midismith::adc_board::app::config::sensors::kSensorIds),
              "Sensor IDs must be unique");
static_assert(midismith::adc_board::app::config::sensors::validation::IsContiguousFrom1(
                  midismith::adc_board::app::config::sensors::kSensorIds),
              "Sensor IDs must be contiguous from 1");
static_assert(midismith::adc_board::app::config::sensors::validation::AreUnique(
                  midismith::adc_board::app::config::sensors::kAnalogRankSensorIds),
              "Analog rank mapping must not contain duplicates");
static_assert(midismith::adc_board::app::config::sensors::validation::AllInSet(
                  midismith::adc_board::app::config::sensors::kAnalogRankSensorIds,
                  midismith::adc_board::app::config::sensors::kSensorIds),
              "Analog rank mapping must reference valid IDs");
static_assert(midismith::adc_board::app::config::sensors::validation::AllInSet(
                  midismith::adc_board::app::config::sensors::kSensorIds,
                  midismith::adc_board::app::config::sensors::kAnalogRankSensorIds),
              "Analog rank mapping must cover all sensor IDs");
