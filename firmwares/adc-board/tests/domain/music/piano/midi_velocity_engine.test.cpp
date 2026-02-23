#if defined(UNIT_TESTS)

#include "domain/music/piano/midi_velocity_engine.hpp"

#include <catch2/catch_test_macros.hpp>

#include "domain/sensors/sensor_state.hpp"

namespace {

class RecordingKeyActionHandler final
    : public midismith::adc_board::domain::music::piano::KeyActionRequirements {
 public:
  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    ++note_on_calls;
    last_velocity = velocity;
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    (void) release_velocity;
  }

  int note_on_calls = 0;
  midismith::midi::Velocity last_velocity = 0u;
};

class RecordingVelocityMapper final
    : public midismith::adc_board::domain::music::piano::velocity::VelocityMapperRequirements {
 public:
  static void Reset() noexcept {
    map_calls = 0;
    last_speed_m_per_s = 0.0f;
  }

  midismith::midi::Velocity Map(float speed_m_per_s) noexcept override {
    last_speed_m_per_s = speed_m_per_s;
    ++map_calls;
    return static_cast<midismith::midi::Velocity>(23u);
  }

  static inline int map_calls = 0;
  static inline float last_speed_m_per_s = 0.0f;
};

struct TestContext {
  midismith::adc_board::domain::sensors::SensorState& sensor;
};

}  // namespace

TEST_CASE("The MidiVelocityEngine class") {
  using Constant64Mapper =
      midismith::adc_board::domain::music::piano::velocity::ConstantVelocityMapper<64u>;
  using DefaultEngine =
      midismith::adc_board::domain::music::piano::MidiVelocityEngine<Constant64Mapper, 0.5f, 0.2f,
                                                                     0.1f, 0.3f, 0.4f>;
  using CustomMapperEngine =
      midismith::adc_board::domain::music::piano::MidiVelocityEngine<RecordingVelocityMapper, 0.5f,
                                                                     0.2f, 0.1f, 0.3f, 0.4f>;

  SECTION("The Execute() method") {
    SECTION("When crossing letoff then strike with the default mapper") {
      SECTION("Should emit NoteOn and update SensorState") {
        midismith::adc_board::domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        DefaultEngine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 1);
        REQUIRE(handler.last_velocity == static_cast<midismith::midi::Velocity>(64u));
        REQUIRE(sensor.last_midi_velocity == static_cast<std::uint8_t>(64u));
        REQUIRE(sensor.is_note_on == true);
      }
    }

    SECTION("When the motion reverses above drop after latching") {
      SECTION("Should not emit NoteOn") {
        midismith::adc_board::domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        DefaultEngine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);

        sensor.last_hammer_speed_m_per_s = +0.5f;
        engine.Execute(0.31f, ctx);

        REQUIRE(handler.note_on_calls == 0);
        REQUIRE(sensor.is_note_on == false);
      }
    }

    SECTION("When re-armed after a strike") {
      SECTION("Should allow a second strike without a NoteOff") {
        midismith::adc_board::domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        DefaultEngine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);

        sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 1);
        REQUIRE(sensor.is_note_on == true);

        sensor.last_hammer_speed_m_per_s = +0.5f;
        engine.Execute(0.41f, ctx);

        sensor.last_hammer_speed_m_per_s = -1.0f;
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 2);
        REQUIRE(sensor.is_note_on == true);
        REQUIRE(sensor.last_midi_velocity == static_cast<std::uint8_t>(64u));
      }
    }

    SECTION("When configured with a custom mapper type") {
      SECTION("Should use the custom mapper output to emit NoteOn") {
        midismith::adc_board::domain::sensors::SensorState sensor{};
        TestContext ctx{sensor};
        CustomMapperEngine engine{};
        RecordingKeyActionHandler handler{};
        engine.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        sensor.last_hammer_speed_m_per_s = -1.0f;
        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        REQUIRE(handler.note_on_calls == 1);
        REQUIRE(handler.last_velocity == static_cast<midismith::midi::Velocity>(23u));
        REQUIRE(RecordingVelocityMapper::map_calls == 1);
        REQUIRE(RecordingVelocityMapper::last_speed_m_per_s == 1.0f);
      }
    }
  }
}

#endif
