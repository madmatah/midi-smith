#if defined(UNIT_TESTS)

#include "domain/music/piano/note_release_detector_stage.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "domain/sensors/sensor_state.hpp"

namespace {

class RecordingKeyActionHandler final : public domain::music::piano::KeyActionRequirements {
 public:
  void OnNoteOn(domain::music::Velocity velocity) noexcept override {
    (void) velocity;
  }

  void OnNoteOff(domain::music::Velocity release_velocity) noexcept override {
    ++note_off_calls;
    last_release_velocity = release_velocity;
  }

  int note_off_calls = 0;
  domain::music::Velocity last_release_velocity = 0u;
};

class RecordingVelocityMapper final : public domain::music::piano::VelocityMapperRequirements {
 public:
  domain::music::Velocity Map(float speed_m_per_s) noexcept override {
    last_hammer_speed_m_per_s = speed_m_per_s;
    return (speed_m_per_s > 0.0f) ? static_cast<domain::music::Velocity>(42u)
                                  : static_cast<domain::music::Velocity>(1u);
  }

  float last_hammer_speed_m_per_s = 0.0f;
};

struct TestContext {
  domain::sensors::SensorState& sensor;
};

}  // namespace

TEST_CASE("The NoteReleaseDetectorStage class") {
  using Catch::Matchers::WithinAbs;
  using Stage = domain::music::piano::NoteReleaseDetectorStage<0.5f>;

  SECTION("The note release detection") {
    SECTION("When note is on and position crosses the threshold upwards") {
      SECTION("Should emit NoteOff and clear is_note_on") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Stage stage{};
        RecordingKeyActionHandler handler{};
        RecordingVelocityMapper mapper{};
        stage.SetKeyActionHandler(&handler);
        stage.SetVelocityMapper(&mapper);

        sensor.is_note_on = true;
        sensor.last_hammer_speed_m_per_s = 0.25f;
        stage.Execute(0.49f, ctx);

        sensor.last_hammer_speed_m_per_s = 0.0f;
        stage.Execute(0.51f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE_THAT(mapper.last_hammer_speed_m_per_s, WithinAbs(0.25f, 0.0001f));
        REQUIRE(handler.last_release_velocity == static_cast<domain::music::Velocity>(42u));
        REQUIRE(sensor.is_note_on == false);
      }
    }

    SECTION("When speed becomes gated at the threshold") {
      SECTION("Should map the last latched positive speed") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Stage stage{};
        RecordingKeyActionHandler handler{};
        RecordingVelocityMapper mapper{};
        stage.SetKeyActionHandler(&handler);
        stage.SetVelocityMapper(&mapper);

        sensor.is_note_on = true;
        sensor.last_hammer_speed_m_per_s = 0.10f;
        stage.Execute(0.40f, ctx);

        sensor.last_hammer_speed_m_per_s = 0.35f;
        stage.Execute(0.45f, ctx);

        sensor.last_hammer_speed_m_per_s = 0.0f;
        stage.Execute(0.51f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE_THAT(mapper.last_hammer_speed_m_per_s, WithinAbs(0.35f, 0.0001f));
      }
    }

    SECTION("When note is off") {
      SECTION("Should not emit NoteOff") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Stage stage{};
        RecordingKeyActionHandler handler{};
        RecordingVelocityMapper mapper{};
        stage.SetKeyActionHandler(&handler);
        stage.SetVelocityMapper(&mapper);

        sensor.is_note_on = false;
        sensor.last_hammer_speed_m_per_s = 0.25f;
        stage.Execute(0.49f, ctx);
        stage.Execute(0.51f, ctx);

        REQUIRE(handler.note_off_calls == 0);
        REQUIRE(sensor.is_note_on == false);
      }
    }
  }
}

#endif
