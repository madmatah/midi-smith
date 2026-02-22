#if defined(UNIT_TESTS)

#include "domain/sensors/sensor_registry.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/sensors/sensor_state.hpp"

TEST_CASE("The SensorRegistry class") {
  SECTION("The constructor") {
    SECTION("When constructing with non-contiguous IDs", "[!mayfail]") {
      SECTION("It should trigger an assertion") {
        midismith::adc_board::domain::sensors::SensorState broken_sensors[] = {{.id = 1},
                                                                               {.id = 3}};
      }
    }
    SECTION("When constructing with IDs not starting at 1", "[!mayfail]") {
      SECTION("It should trigger an assertion") {
        midismith::adc_board::domain::sensors::SensorState broken_sensors[] = {{.id = 2},
                                                                               {.id = 3}};
      }
    }
  }


  SECTION("The FindById() method") {
    SECTION("When the id exists in the registry") {
      SECTION("It should return the matching sensor state") {
        midismith::adc_board::domain::sensors::SensorState s1{.id = 1};
        midismith::adc_board::domain::sensors::SensorState s2{.id = 2};
        midismith::adc_board::domain::sensors::SensorState sensors[] = {s1, s2};
        midismith::adc_board::domain::sensors::SensorRegistry registry(sensors, 2);

        midismith::adc_board::domain::sensors::SensorState* found = registry.FindById(2);

        REQUIRE(found != nullptr);
        REQUIRE(found->id == 2);
      }
    }

    SECTION("When the id does not exist in the registry") {
      SECTION("It should return nullptr") {
        midismith::adc_board::domain::sensors::SensorState s1{.id = 1};
        midismith::adc_board::domain::sensors::SensorState sensors[] = {s1};
        midismith::adc_board::domain::sensors::SensorRegistry registry(sensors, 1);

        midismith::adc_board::domain::sensors::SensorState* found = registry.FindById(2);

        REQUIRE(found == nullptr);
      }
    }

    SECTION("When the id is zero") {
      SECTION("It should return nullptr") {
        midismith::adc_board::domain::sensors::SensorState s1{.id = 1};
        midismith::adc_board::domain::sensors::SensorState sensors[] = {s1};
        midismith::adc_board::domain::sensors::SensorRegistry registry(sensors, 1);

        midismith::adc_board::domain::sensors::SensorState* found = registry.FindById(0);

        REQUIRE(found == nullptr);
      }
    }
  }
}


#endif
