#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace app::telemetry {

constexpr std::uint32_t kSensorRttMagic = 0x54545253u;
constexpr std::uint8_t kSensorRttVersion = 2u;

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
  float shank_position_norm = 0.0f;
  float shank_position_mm = 0.0f;
  float shank_speed_m_per_s = 0.0f;
  float hammer_speed_m_per_s = 0.0f;
};
static_assert(std::is_trivially_copyable_v<SensorRttDataPayload>,
              "Sensor RTT data payload must be trivially copyable");

struct SensorRttDataFrame {
  SensorRttFrameHeader header{};
  SensorRttDataPayload payload{};
};
static_assert(std::is_trivially_copyable_v<SensorRttDataFrame>,
              "Sensor RTT data frame must be trivially copyable");

struct SensorRttMetricDescriptor {
  std::uint8_t metric_id = 0u;
  SensorRttValueType value_type = SensorRttValueType::kFloat32;
  std::uint16_t offset_bytes = 0u;
  float suggested_min = 0.0f;
  float suggested_max = 0.0f;
  const char* name = "";
};

constexpr std::size_t SensorRttMetricNameLengthBytes(
    const SensorRttMetricDescriptor& metric) noexcept {
  std::size_t length = 0u;
  if (metric.name == nullptr) {
    return length;
  }
  while (metric.name[length] != '\0') {
    ++length;
  }
  return length;
}

constexpr std::array kSensorRttDataPayloadMetrics = {
    SensorRttMetricDescriptor{
        0u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, adc_raw)),
        0.0f,
        65535.0f,
        "ADC (raw)",
    },
    SensorRttMetricDescriptor{
        1u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, adc_filtered)),
        0.0f,
        65535.0f,
        "ADC (filtered)",
    },
    SensorRttMetricDescriptor{
        2u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, current_ma)),
        0.0f,
        1.0f,
        "Current (mA)",
    },
    SensorRttMetricDescriptor{
        3u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, shank_position_norm)),
        0.0f,
        1.0f,
        "Shank position (normalized)",
    },
    SensorRttMetricDescriptor{
        4u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, shank_position_mm)),
        0.0f,
        12.0f,
        "Shank position (mm)",
    },
    SensorRttMetricDescriptor{
        5u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, shank_speed_m_per_s)),
        -0.35f,
        0.15f,
        "Shank speed (m/s)",
    },
    SensorRttMetricDescriptor{
        6u,
        SensorRttValueType::kFloat32,
        static_cast<std::uint16_t>(offsetof(SensorRttDataPayload, hammer_speed_m_per_s)),
        -2.5f,
        1.0f,
        "Hammer speed (m/s)",
    },
};

constexpr std::size_t kMetricCount = kSensorRttDataPayloadMetrics.size();
constexpr std::size_t kExpectedPayloadSize = (kMetricCount * sizeof(float)) + 4u;
constexpr std::size_t kExpectedFrameSize = sizeof(SensorRttFrameHeader) + kExpectedPayloadSize;

static_assert(sizeof(SensorRttDataPayload) == kExpectedPayloadSize,
              "Sensor RTT payload size must match 4-byte prefix + one float per metric");
static_assert(sizeof(SensorRttDataFrame) == kExpectedFrameSize,
              "Sensor RTT frame size must match header size + expected payload size");
static_assert(sizeof(SensorRttDataPayload) >= 4u,
              "Sensor RTT payload must include the 4-byte prefix (sensor_id + padding)");
static_assert((sizeof(SensorRttDataPayload) - 4u) % sizeof(float) == 0u,
              "Sensor RTT payload content after the 4-byte prefix must be a float array");
static_assert(((sizeof(SensorRttDataPayload) - 4u) / sizeof(float)) == kMetricCount,
              "Sensor RTT payload float count must match metric descriptor count");

constexpr std::size_t kSensorRttSchemaPrefixSizeBytes = 8u;
constexpr std::size_t kSensorRttSchemaMetricHeaderSizeBytes = 13u;

constexpr std::size_t SensorRttMetricCount() noexcept {
  return kMetricCount;
}

constexpr std::size_t SensorRttSchemaPayloadSizeBytes() noexcept {
  std::size_t payload_size_bytes = kSensorRttSchemaPrefixSizeBytes;
  for (const SensorRttMetricDescriptor& metric : kSensorRttDataPayloadMetrics) {
    payload_size_bytes += kSensorRttSchemaMetricHeaderSizeBytes;
    payload_size_bytes += SensorRttMetricNameLengthBytes(metric);
  }
  return payload_size_bytes;
}

constexpr std::size_t SensorRttSchemaFrameSizeBytes() noexcept {
  return sizeof(SensorRttFrameHeader) + SensorRttSchemaPayloadSizeBytes();
}

constexpr std::size_t SensorRttFrameHeaderSizeBytes() noexcept {
  return sizeof(SensorRttFrameHeader);
}

constexpr std::size_t SensorRttDataPayloadSizeBytes() noexcept {
  return sizeof(SensorRttDataPayload);
}

constexpr std::size_t SensorRttDataFrameSizeBytes() noexcept {
  return sizeof(SensorRttDataFrame);
}

namespace sensor_rtt_protocol_detail {

constexpr bool AreMetricIdsSequential() noexcept {
  for (std::size_t index = 0u; index < kSensorRttDataPayloadMetrics.size(); ++index) {
    if (kSensorRttDataPayloadMetrics[index].metric_id != index) {
      return false;
    }
  }
  return true;
}

constexpr bool AreMetricOffsetsStrictlyIncreasing() noexcept {
  if (kSensorRttDataPayloadMetrics.empty()) {
    return true;
  }
  for (std::size_t index = 1u; index < kSensorRttDataPayloadMetrics.size(); ++index) {
    if (kSensorRttDataPayloadMetrics[index - 1u].offset_bytes >=
        kSensorRttDataPayloadMetrics[index].offset_bytes) {
      return false;
    }
  }
  return true;
}

constexpr bool AreMetricDefinitionsValidForPayload() noexcept {
  for (const SensorRttMetricDescriptor& metric : kSensorRttDataPayloadMetrics) {
    if (metric.name == nullptr) {
      return false;
    }
    if (SensorRttMetricNameLengthBytes(metric) == 0u) {
      return false;
    }
    if (SensorRttMetricNameLengthBytes(metric) > std::numeric_limits<std::uint8_t>::max()) {
      return false;
    }
    if (metric.value_type != SensorRttValueType::kFloat32) {
      return false;
    }
    const std::size_t value_offset_bytes = metric.offset_bytes;
    if ((value_offset_bytes + sizeof(float)) > sizeof(SensorRttDataPayload)) {
      return false;
    }
  }
  return true;
}

constexpr bool AreMetricSuggestedRangesValid() noexcept {
  for (const SensorRttMetricDescriptor& metric : kSensorRttDataPayloadMetrics) {
    if (!(metric.suggested_min <= metric.suggested_max)) {
      return false;
    }
  }
  return true;
}

}  // namespace sensor_rtt_protocol_detail

static_assert(SensorRttDataPayloadSizeBytes() <= std::numeric_limits<std::uint16_t>::max(),
              "Data payload size must fit in uint16_t");
static_assert(SensorRttFrameHeaderSizeBytes() <= std::numeric_limits<std::uint16_t>::max(),
              "Frame header size must fit in uint16_t");
static_assert(SensorRttMetricCount() <= std::numeric_limits<std::uint8_t>::max(),
              "Metric count must fit in uint8_t");
static_assert(SensorRttSchemaPayloadSizeBytes() <= std::numeric_limits<std::uint16_t>::max(),
              "Schema payload size must fit in uint16_t");
static_assert(sensor_rtt_protocol_detail::AreMetricIdsSequential(),
              "Metric IDs must stay sequential for backward compatibility");
static_assert(sensor_rtt_protocol_detail::AreMetricOffsetsStrictlyIncreasing(),
              "Metric offsets must stay strictly increasing");
static_assert(sensor_rtt_protocol_detail::AreMetricDefinitionsValidForPayload(),
              "Metric definitions must remain valid for SensorRttDataPayload");
static_assert(sensor_rtt_protocol_detail::AreMetricSuggestedRangesValid(),
              "Metric suggested ranges must satisfy min <= max");

}  // namespace app::telemetry
