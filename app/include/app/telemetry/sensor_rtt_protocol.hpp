#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace app::telemetry {

constexpr std::uint32_t kSensorRttMagic = 0x54545253u;
constexpr std::uint8_t kSensorRttVersion = 1u;

enum class SensorRttFrameKind : std::uint8_t {
  kSchema = 1u,
  kData = 2u,
};

enum class SensorRttValueType : std::uint8_t {
  kFloat32 = 1u,
};

struct SensorRttFrameHeader {
  std::uint32_t magic = kSensorRttMagic;
  std::uint8_t version = kSensorRttVersion;
  std::uint8_t kind = static_cast<std::uint8_t>(SensorRttFrameKind::kData);
  std::uint16_t payload_size_bytes = 0u;
  std::uint32_t seq = 0u;
  std::uint32_t timestamp_us = 0u;
};
static_assert(sizeof(SensorRttFrameHeader) == 16u, "Expected 16-byte sensor RTT frame header");
static_assert(std::is_trivially_copyable_v<SensorRttFrameHeader>,
              "Sensor RTT header must be trivially copyable");

struct SensorRttDataPayload {
  std::uint8_t sensor_id = 0u;
  std::uint8_t _padding[3]{0u, 0u, 0u};
  float adc_raw = 0.0f;
  float adc_filtered = 0.0f;
  float current_ma = 0.0f;
  float position_norm = 0.0f;
  float speed_m_per_s = 0.0f;
};
static_assert(sizeof(SensorRttDataPayload) == 24u, "Expected 24-byte sensor RTT data payload");
static_assert(std::is_trivially_copyable_v<SensorRttDataPayload>,
              "Sensor RTT data payload must be trivially copyable");

struct SensorRttDataFrame {
  SensorRttFrameHeader header{};
  SensorRttDataPayload payload{};
};
static_assert(sizeof(SensorRttDataFrame) == 40u, "Expected 40-byte sensor RTT data frame");
static_assert(std::is_trivially_copyable_v<SensorRttDataFrame>,
              "Sensor RTT data frame must be trivially copyable");

constexpr std::size_t SensorRttFrameHeaderSizeBytes() noexcept {
  return sizeof(SensorRttFrameHeader);
}

constexpr std::size_t SensorRttDataPayloadSizeBytes() noexcept {
  return sizeof(SensorRttDataPayload);
}

constexpr std::size_t SensorRttDataFrameSizeBytes() noexcept {
  return sizeof(SensorRttDataFrame);
}

}  // namespace app::telemetry
