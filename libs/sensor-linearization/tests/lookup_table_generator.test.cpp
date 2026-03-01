#if defined(UNIT_TESTS)

#include "sensor-linearization/lookup_table_generator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "sensor-linearization/sensor_response_curve.hpp"

using namespace midismith::sensor_linearization;
using Catch::Matchers::WithinAbs;

TEST_CASE("The LookupTableGenerator class", "[sensor][linearization]") {
  constexpr std::size_t kLutSize = 256u;
  SensorLookupTable<kLutSize> lut{};

  SECTION("Generate()") {
    SECTION("When the master signature is invalid (less than 2 points)") {
      SECTION("Should return kInvalidMasterSignature and fill a linear fallback table") {
        SensorResponseCurve master(nullptr, 0);
        SensorCalibration calib{.rest_current_ma = 0.5f,
                                .strike_current_ma = 1.5f,
                                .rest_distance_mm = 10.0f,
                                .strike_distance_mm = 1.0f};

        auto result = LookupTableGenerator::Generate(master, calib, lut);

        REQUIRE(result.status == LookupTableGenerationStatus::kInvalidMasterSignature);
        REQUIRE_THAT(lut[0], WithinAbs(1.0f, 0.0001f));
        REQUIRE_THAT(lut[kLutSize - 1], WithinAbs(0.0f, 0.0001f));
      }
    }

    SECTION("When the calibration is invalid") {
      SECTION("Due to distance range (strike >= rest)") {
        const SensorResponsePoint points[] = {{1.0f, 1.0f}, {10.0f, 0.1f}};
        SensorResponseCurve master(points, 2);
        SensorCalibration calib{.rest_current_ma = 0.5f,
                                .strike_current_ma = 1.5f,
                                .rest_distance_mm = 5.0f,
                                .strike_distance_mm = 5.0f};

        auto result = LookupTableGenerator::Generate(master, calib, lut);

        REQUIRE(result.status == LookupTableGenerationStatus::kInvalidCalibration);
        REQUIRE_THAT(lut[0], WithinAbs(1.0f, 0.0001f));
      }

      SECTION("Due to current range (rest == strike)") {
        const SensorResponsePoint points[] = {{1.0f, 1.0f}, {10.0f, 0.1f}};
        SensorResponseCurve master(points, 2);
        SensorCalibration calib{.rest_current_ma = 1.0f,
                                .strike_current_ma = 1.0f,
                                .rest_distance_mm = 10.0f,
                                .strike_distance_mm = 1.0f};

        auto result = LookupTableGenerator::Generate(master, calib, lut);

        REQUIRE(result.status == LookupTableGenerationStatus::kInvalidCalibration);
        REQUIRE_THAT(lut[0], WithinAbs(1.0f, 0.0001f));
      }
    }

    SECTION("When inputs are valid") {
      SECTION("Should return kOk and compute a non-linear but monotone table") {
        const SensorResponsePoint points[] = {{0.0f, 1.0f}, {5.0f, 0.5f}, {10.0f, 0.25f}};
        SensorResponseCurve master(points, 3);
        SensorCalibration calib{.rest_current_ma = 0.5f,
                                .strike_current_ma = 1.5f,
                                .rest_distance_mm = 10.0f,
                                .strike_distance_mm = 1.0f};

        auto result = LookupTableGenerator::Generate(master, calib, lut);

        REQUIRE(result.status == LookupTableGenerationStatus::kOk);
        REQUIRE(result.configuration.lookup_table == &lut);

        REQUIRE_THAT(lut[0], WithinAbs(1.0f, 0.0001f));
        REQUIRE_THAT(lut[kLutSize - 1], WithinAbs(0.0f, 0.01f));
      }
    }
  }
}

#endif
