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

float ReadF32LE(const std::uint8_t* bytes) noexcept {
  const std::uint32_t raw_bits = ReadU32LE(bytes);
  float value = 0.0f;
  std::memcpy(&value, &raw_bits, sizeof(value));
  return value;
}

}  // namespace

TEST_CASE("The BuildSensorRttSchemaFrame function", "[app][telemetry]") {
  SECTION("BuildSensorRttSchemaFrame()") {
    SECTION("When the output buffer has exactly the required size") {
      constexpr std::uint8_t kSensorId = 7u;
      constexpr std::uint32_t kTimestampUs = 123'456u;
      std::array<std::uint8_t,
                 midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes()>
          frame_bytes{};

      const std::size_t written = midismith::adc_board::app::telemetry::BuildSensorRttSchemaFrame(
          kSensorId, kTimestampUs, frame_bytes);

      SECTION("Should return the total schema frame size") {
        REQUIRE(written == frame_bytes.size());
        REQUIRE(written == midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes());
      }

      SECTION("Should write a valid frame header") {
        const auto* bytes = frame_bytes.data();
        REQUIRE(ReadU32LE(&bytes[0]) == midismith::adc_board::app::telemetry::kSensorRttMagic);
        REQUIRE(bytes[4] == midismith::adc_board::app::telemetry::kSensorRttVersion);
        REQUIRE(bytes[5] == static_cast<std::uint8_t>(
                                midismith::adc_board::app::telemetry::SensorRttFrameKind::kSchema));
        REQUIRE(ReadU16LE(&bytes[6]) ==
                midismith::adc_board::app::telemetry::SensorRttSchemaPayloadSizeBytes());
        REQUIRE(ReadU32LE(&bytes[8]) == 0u);
        REQUIRE(ReadU32LE(&bytes[12]) == kTimestampUs);
      }

      SECTION("Should write the correct schema prefix") {
        const auto* bytes = frame_bytes.data();
        std::size_t cursor = midismith::adc_board::app::telemetry::SensorRttFrameHeaderSizeBytes();
        REQUIRE(bytes[cursor++] == kSensorId);
        REQUIRE(bytes[cursor++] == midismith::adc_board::app::telemetry::SensorRttMetricCount());
        REQUIRE(ReadU16LE(&bytes[cursor]) ==
                midismith::adc_board::app::telemetry::SensorRttDataPayloadSizeBytes());
        cursor += 2u;
        REQUIRE(ReadU16LE(&bytes[cursor]) ==
                midismith::adc_board::app::telemetry::SensorRttFrameHeaderSizeBytes());
        cursor += 2u;
        REQUIRE(ReadU16LE(&bytes[cursor]) == 0u);
      }

      SECTION("Should write the correct binary encoding for every metric descriptor") {
        const auto* bytes = frame_bytes.data();
        std::size_t cursor = midismith::adc_board::app::telemetry::SensorRttFrameHeaderSizeBytes() +
                             midismith::adc_board::app::telemetry::kSensorRttSchemaPrefixSizeBytes;

        for (const auto& metric :
             midismith::adc_board::app::telemetry::kSensorRttDataPayloadMetrics) {
          REQUIRE(bytes[cursor++] == metric.metric_id);
          REQUIRE(bytes[cursor++] == static_cast<std::uint8_t>(metric.value_type));
          REQUIRE(ReadU16LE(&bytes[cursor]) == metric.offset_bytes);
          cursor += 2u;
          REQUIRE(ReadF32LE(&bytes[cursor]) == metric.suggested_min);
          cursor += 4u;
          REQUIRE(ReadF32LE(&bytes[cursor]) == metric.suggested_max);
          cursor += 4u;
          const std::size_t name_length =
              midismith::adc_board::app::telemetry::SensorRttMetricNameLengthBytes(metric);
          REQUIRE(bytes[cursor++] == name_length);
          REQUIRE(std::memcmp(&bytes[cursor], metric.name, name_length) == 0);
          cursor += name_length;
        }

        REQUIRE(cursor == written);
      }
    }

    SECTION("When the output buffer is larger than the required size") {
      SECTION("Should return exactly the schema frame size") {
        constexpr std::size_t kExtraBytes = 16u;
        std::array<std::uint8_t,
                   midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes() +
                       kExtraBytes>
            frame_bytes{};

        const std::size_t written =
            midismith::adc_board::app::telemetry::BuildSensorRttSchemaFrame(3u, 0u, frame_bytes);

        REQUIRE(written == midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes());
      }
    }

    SECTION("When the output buffer is one byte too small") {
      SECTION("Should return zero") {
        std::array<std::uint8_t,
                   midismith::adc_board::app::telemetry::SensorRttSchemaFrameSizeBytes() - 1u>
            frame_bytes{};

        const std::size_t written =
            midismith::adc_board::app::telemetry::BuildSensorRttSchemaFrame(1u, 42u, frame_bytes);

        REQUIRE(written == 0u);
      }
    }
  }
}

#endif
