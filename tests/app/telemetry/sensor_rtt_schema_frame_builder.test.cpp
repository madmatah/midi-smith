#if defined(UNIT_TESTS)

#include "app/telemetry/sensor_rtt_schema_frame_builder.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "app/telemetry/sensor_rtt_protocol.hpp"

namespace {

std::uint16_t ReadU16LE(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint16_t>(bytes[0]) |
         static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[1]) << 8u);
}

std::uint32_t ReadU32LE(const std::uint8_t* bytes) noexcept {
  return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8u) |
         (static_cast<std::uint32_t>(bytes[2]) << 16u) |
         (static_cast<std::uint32_t>(bytes[3]) << 24u);
}

}  // namespace

TEST_CASE("BuildSensorRttSchemaFrame", "[app][telemetry]") {
  SECTION("Builds the schema frame with the expected binary format") {
    constexpr std::uint8_t kSensorId = 7u;
    constexpr std::uint32_t kTimestampUs = 123'456u;
    std::array<std::uint8_t, app::telemetry::SensorRttSchemaFrameSizeBytes()> frame_bytes{};

    const std::size_t written =
        app::telemetry::BuildSensorRttSchemaFrame(kSensorId, kTimestampUs, frame_bytes);

    REQUIRE(written == frame_bytes.size());
    REQUIRE(written == app::telemetry::SensorRttSchemaFrameSizeBytes());

    const auto* bytes = frame_bytes.data();
    REQUIRE(ReadU32LE(&bytes[0]) == app::telemetry::kSensorRttMagic);
    REQUIRE(bytes[4] == app::telemetry::kSensorRttVersion);
    REQUIRE(bytes[5] == static_cast<std::uint8_t>(app::telemetry::SensorRttFrameKind::kSchema));
    REQUIRE(ReadU16LE(&bytes[6]) == app::telemetry::SensorRttSchemaPayloadSizeBytes());
    REQUIRE(ReadU32LE(&bytes[8]) == 0u);
    REQUIRE(ReadU32LE(&bytes[12]) == kTimestampUs);

    std::size_t cursor = app::telemetry::SensorRttFrameHeaderSizeBytes();
    REQUIRE(bytes[cursor++] == kSensorId);
    REQUIRE(bytes[cursor++] == app::telemetry::SensorRttMetricCount());
    REQUIRE(ReadU16LE(&bytes[cursor]) == app::telemetry::SensorRttDataPayloadSizeBytes());
    cursor += 2u;
    REQUIRE(ReadU16LE(&bytes[cursor]) == app::telemetry::SensorRttFrameHeaderSizeBytes());
    cursor += 2u;
    REQUIRE(ReadU16LE(&bytes[cursor]) == 0u);
    cursor += 2u;

    for (const auto& metric : app::telemetry::kSensorRttDataPayloadMetrics) {
      REQUIRE(bytes[cursor++] == metric.metric_id);
      REQUIRE(bytes[cursor++] == static_cast<std::uint8_t>(metric.value_type));
      REQUIRE(ReadU16LE(&bytes[cursor]) == metric.offset_bytes);
      cursor += 2u;

      const std::size_t expected_name_length =
          app::telemetry::SensorRttMetricNameLengthBytes(metric);
      REQUIRE(bytes[cursor++] == expected_name_length);
      REQUIRE(std::memcmp(&bytes[cursor], metric.name, expected_name_length) == 0);
      cursor += expected_name_length;
    }

    REQUIRE(cursor == written);
  }

  SECTION("Returns zero when the output buffer is too small") {
    std::array<std::uint8_t, app::telemetry::SensorRttSchemaFrameSizeBytes() - 1u> frame_bytes{};

    const std::size_t written = app::telemetry::BuildSensorRttSchemaFrame(1u, 42u, frame_bytes);
    REQUIRE(written == 0u);
  }
}

#endif
