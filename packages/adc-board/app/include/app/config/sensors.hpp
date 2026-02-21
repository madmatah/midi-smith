#pragma once

#include <cstddef>
#include <cstdint>

namespace app::config_sensors {

constexpr std::uint8_t kSensorIds[] = {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
                                       12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
constexpr std::size_t kSensorCount = sizeof(kSensorIds) / sizeof(kSensorIds[0]);

// ADC rank to SensorId mapping.
//
// A "rank" is the position inside the regular scan sequence configured in CubeMX (Rank 1..N).
// Each ADC has its own scan sequence:
// - ADC1 has 7 ranks
// - ADC2 has 7 ranks
// - ADC3 has 8 ranks
//
// The firmware updates sensors using SensorId only:
// - For each converted value, we lookup the SensorId for the corresponding rank
// - Then we update the unified 22-sensor group at index (SensorId - 1)
//
// This keeps "SensorId == board labeling (SENx)" stable, even if you later rewire sensors to a
// different ADC and/or change the channel/rank assignments in CubeMX.
constexpr std::size_t kAdc1RankCount = 7;
constexpr std::size_t kAdc2RankCount = 7;
constexpr std::size_t kAdc3RankCount = 8;
static_assert(kAdc1RankCount + kAdc2RankCount + kAdc3RankCount == kSensorCount,
              "ADC rank mapping must cover all sensors");

constexpr std::uint8_t kAdc1SensorIdByRank[kAdc1RankCount] = {1, 3, 5, 7, 9, 11, 12};
constexpr std::uint8_t kAdc2SensorIdByRank[kAdc2RankCount] = {2, 4, 6, 8, 10, 15, 16};
constexpr std::uint8_t kAdc3SensorIdByRank[kAdc3RankCount] = {13, 14, 17, 18, 19, 20, 21, 22};

// Flattened view of all rank mappings.
//
// This array is not used by the acquisition runtime logic.
// It exists as a convenient "single list" that the compile-time validation can check:
// - no duplicates
// - all SensorIds are covered exactly once
constexpr std::uint8_t kAnalogRankSensorIds[kSensorCount] = {
    kAdc1SensorIdByRank[0], kAdc1SensorIdByRank[1], kAdc1SensorIdByRank[2], kAdc1SensorIdByRank[3],
    kAdc1SensorIdByRank[4], kAdc1SensorIdByRank[5], kAdc1SensorIdByRank[6], kAdc2SensorIdByRank[0],
    kAdc2SensorIdByRank[1], kAdc2SensorIdByRank[2], kAdc2SensorIdByRank[3], kAdc2SensorIdByRank[4],
    kAdc2SensorIdByRank[5], kAdc2SensorIdByRank[6], kAdc3SensorIdByRank[0], kAdc3SensorIdByRank[1],
    kAdc3SensorIdByRank[2], kAdc3SensorIdByRank[3], kAdc3SensorIdByRank[4], kAdc3SensorIdByRank[5],
    kAdc3SensorIdByRank[6], kAdc3SensorIdByRank[7],
};

}  // namespace app::config_sensors
