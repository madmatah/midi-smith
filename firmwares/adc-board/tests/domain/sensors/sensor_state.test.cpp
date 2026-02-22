#if defined(UNIT_TESTS)

#include "domain/sensors/sensor_state.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

TEST_CASE("The SensorState struct") {
  using Catch::Matchers::WithinAbs;

  SECTION("The default initialization") {
    SECTION("When value-initialized") {
      SECTION("Should zero initialize all fields") {
        midismith::adc_board::domain::sensors::SensorState state{};

        REQUIRE(state.id == 0);
        REQUIRE(state.is_note_on == false);
        REQUIRE(state.last_raw_value == 0);
        REQUIRE(state.last_midi_velocity == 0);
        REQUIRE_THAT(state.last_processed_value, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(state.last_shank_position_norm, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(state.last_shank_position_mm, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(state.last_shank_speed_m_per_s, WithinAbs(0.0f, 0.001f));
        REQUIRE_THAT(state.last_hammer_speed_m_per_s, WithinAbs(0.0f, 0.001f));
        REQUIRE(state.last_timestamp_ticks == 0);
      }
    }
  }
}

#endif
