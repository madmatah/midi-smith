#if defined(UNIT_TESTS)

#include "domain/sensors/sensor_member_reader.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

struct MockSensor {
  float value_a;
  float value_b;
};

struct MockContext {
  MockSensor sensor;
};

}  // namespace

TEST_CASE("The SensorMemberReader class") {
  using Catch::Matchers::WithinAbs;
  using domain::sensors::SensorMemberReader;

  SECTION("The operator()") {
    SECTION("When reading value_a") {
      SECTION("Should return the value from the member value_a") {
        const MockContext ctx{.sensor = {.value_a = 42.5f, .value_b = 10.0f}};
        const SensorMemberReader<&MockSensor::value_a> reader;

        const float result = reader(ctx);

        REQUIRE_THAT(result, WithinAbs(42.5f, 0.001f));
      }
    }

    SECTION("When reading value_b") {
      SECTION("Should return the value from the member value_b") {
        const MockContext ctx{.sensor = {.value_a = 42.5f, .value_b = 10.2f}};
        const SensorMemberReader<&MockSensor::value_b> reader;

        const float result = reader(ctx);

        REQUIRE_THAT(result, WithinAbs(10.2f, 0.001f));
      }
    }
  }
}

#endif
