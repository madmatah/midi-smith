#if defined(UNIT_TESTS)

#include "domain/music/piano/midi_velocity_engine.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/sensors/sensor_state.hpp"

namespace {

class RecordingKeyActionHandler final : public domain::music::piano::KeyActionRequirements {
 public:
  void OnNoteOn(domain::music::Velocity velocity) noexcept override {
    ++note_on_calls;
    last_velocity = velocity;
  }

  void OnNoteOff(domain::music::Velocity release_velocity) noexcept override {
    (void) release_velocity;
  }

  int note_on_calls = 0;
  domain::music::Velocity last_velocity = 0u;
};

struct TestContext {
  domain::sensors::SensorState& sensor;
};

}  // namespace

TEST_CASE("The MidiVelocityEngine class") {
  using Engine = domain::music::piano::MidiVelocityEngine<0.5f, 0.2f, 0.1f, 0.3f, 0.4f>;

  SECTION("The Execute() method") {
    SECTION("When crossing letoff then strike") {
      SECTION("Should emit NoteOn and update SensorState") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Engine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 1);
        REQUIRE(handler.last_velocity == static_cast<domain::music::Velocity>(58u));
        REQUIRE(sensor.last_midi_velocity == static_cast<std::uint8_t>(58u));
        REQUIRE(sensor.is_note_on == true);
      }
    }

    SECTION("When the motion reverses above drop after latching") {
      SECTION("Should not emit NoteOn") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Engine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);

        sensor.last_speed_m_per_s = +0.5f;
        engine.Execute(0.31f, ctx);

        REQUIRE(handler.note_on_calls == 0);
        REQUIRE(sensor.is_note_on == false);
      }
    }

    SECTION("When re-armed after a strike") {
      SECTION("Should allow a second strike without a NoteOff") {
        domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        Engine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 1);
        REQUIRE(sensor.is_note_on == true);

        sensor.last_speed_m_per_s = +0.5f;
        engine.Execute(0.41f, ctx);

        sensor.last_speed_m_per_s = -1.0f;
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 2);
        REQUIRE(sensor.is_note_on == true);
        REQUIRE(sensor.last_midi_velocity == static_cast<std::uint8_t>(58u));
      }
    }
  }
}

#endif
