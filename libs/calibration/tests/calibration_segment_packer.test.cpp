#if defined(UNIT_TESTS)

#include "calibration/calibration_segment_packer.hpp"

#include <array>

#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::calibration::CalibrationSegmentPacker;
using midismith::calibration::SensorCalibration;
using Packer2 = CalibrationSegmentPacker<2u>;
using Packer3 = CalibrationSegmentPacker<3u>;

SensorCalibration MakeCalibration(float rest_current_ma, float strike_current_ma,
                                  float rest_distance_mm, float strike_distance_mm) {
  SensorCalibration calibration{};
  calibration.rest_current_ma = rest_current_ma;
  calibration.strike_current_ma = strike_current_ma;
  calibration.rest_distance_mm = rest_distance_mm;
  calibration.strike_distance_mm = strike_distance_mm;
  return calibration;
}

}

TEST_CASE("The CalibrationSegmentPacker class", "[calibration][segment]") {
  SECTION("The ComputeTotalSegments() method") {
    SECTION("When the packer is configured with two calibrations per segment") {
      SECTION("Should compute segment count from the template parameter") {
        REQUIRE(Packer2::ComputeTotalSegments(5u) == 3u);
      }
    }

    SECTION("When the sensor count is one") {
      SECTION("Should return one segment") {
        REQUIRE(Packer3::ComputeTotalSegments(1u) == 1u);
      }
    }

    SECTION("When the sensor count is twenty two") {
      SECTION("Should return eight segments") {
        REQUIRE(Packer3::ComputeTotalSegments(22u) == 8u);
      }
    }
  }

  SECTION("The PackSegment() and UnpackSegment() methods") {
    SECTION("When all sensors fit within full segments") {
      SECTION("Should preserve all calibration values after round trip") {
        std::array<SensorCalibration, 6> source{
            MakeCalibration(0.1f, 0.5f, 7.0f, 1.9f), MakeCalibration(0.2f, 0.6f, 7.0f, 1.9f),
            MakeCalibration(0.3f, 0.7f, 7.0f, 1.9f), MakeCalibration(0.4f, 0.8f, 7.0f, 1.9f),
            MakeCalibration(0.5f, 0.9f, 7.0f, 1.9f), MakeCalibration(0.6f, 1.0f, 7.0f, 1.9f)};
        std::array<SensorCalibration, 6> destination{};

        std::array<std::uint8_t, Packer3::kSegmentPayloadSizeBytes> payload0{};
        std::array<std::uint8_t, Packer3::kSegmentPayloadSizeBytes> payload1{};
        Packer3::PackSegment(source.data(), source.size(), 0u, payload0.data());
        Packer3::PackSegment(source.data(), source.size(), 1u, payload1.data());

        Packer3::UnpackSegment(payload0.data(), 0u, destination.data(), destination.size());
        Packer3::UnpackSegment(payload1.data(), 1u, destination.data(), destination.size());

        REQUIRE(destination == source);
      }
    }

    SECTION("When the last segment is partial") {
      SECTION("Should write only available sensors and keep trailing slots zeroed") {
        std::array<SensorCalibration, 4> source{
            MakeCalibration(0.1f, 0.5f, 7.0f, 1.9f), MakeCalibration(0.2f, 0.6f, 7.0f, 1.9f),
            MakeCalibration(0.3f, 0.7f, 7.0f, 1.9f), MakeCalibration(0.4f, 0.8f, 7.0f, 1.9f)};
        std::array<SensorCalibration, 4> destination{};
        std::array<std::uint8_t, Packer3::kSegmentPayloadSizeBytes> payload{};

        Packer3::PackSegment(source.data(), source.size(), 1u, payload.data());
        Packer3::UnpackSegment(payload.data(), 1u, destination.data(), destination.size());

        REQUIRE(destination[3] == source[3]);
      }
    }

    SECTION("When there is only one sensor") {
      SECTION("Should pack and unpack a single calibration entry") {
        std::array<SensorCalibration, 1> source{MakeCalibration(0.12f, 0.62f, 7.0f, 1.9f)};
        std::array<SensorCalibration, 1> destination{};
        std::array<std::uint8_t, Packer3::kSegmentPayloadSizeBytes> payload{};

        Packer3::PackSegment(source.data(), source.size(), 0u, payload.data());
        Packer3::UnpackSegment(payload.data(), 0u, destination.data(), destination.size());

        REQUIRE(destination == source);
      }
    }
  }
}

#endif
