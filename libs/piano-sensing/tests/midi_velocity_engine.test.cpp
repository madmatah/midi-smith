#if defined(UNIT_TESTS)

#include "piano-sensing/midi_velocity_engine.hpp"

#include <catch2/catch_test_macros.hpp>

#include <fakeit.hpp>

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

class RecordingVelocityMapper final
    : public midismith::piano_sensing::velocity::VelocityMapperRequirements {
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
  struct Sensor {
    float last_hammer_speed_m_per_s = 0.0f;
    std::uint8_t last_midi_velocity = 0u;
    bool is_note_on = false;
  } sensor;
};

}  // namespace

TEST_CASE("The MidiVelocityEngine class") {
  using Constant64Mapper = midismith::piano_sensing::velocity::ConstantVelocityMapper<64u>;
  using DefaultEngine =
      midismith::piano_sensing::MidiVelocityEngine<Constant64Mapper, 0.5f, 0.2f, 0.1f, 0.3f, 0.4f>;
  using CustomMapperEngine =
      midismith::piano_sensing::MidiVelocityEngine<RecordingVelocityMapper, 0.5f, 0.2f, 0.1f, 0.3f,
                                                   0.4f>;

  SECTION("The Execute() method") {
    SECTION("When crossing letoff then strike with the default mapper") {
      SECTION("Should emit NoteOn and update SensorState") {
        TestContext ctx{};
        DefaultEngine engine{};
        Mock<midismith::piano_sensing::KeyActionRequirements> handler_mock;
        Fake(fakeit_Method(handler_mock, OnNoteOn));

        engine.SetKeyActionHandler(&handler_mock.get());

        ctx.sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        Verify(fakeit_Method(handler_mock, OnNoteOn).Using(static_cast<midismith::midi::Velocity>(64u)))
            .Once();
        REQUIRE(ctx.sensor.last_midi_velocity == static_cast<std::uint8_t>(64u));
        REQUIRE(ctx.sensor.is_note_on == true);
      }
    }

    SECTION("When the motion reverses above drop after latching") {
      SECTION("Should not emit NoteOn") {
        TestContext ctx{};
        DefaultEngine engine{};
        Mock<midismith::piano_sensing::KeyActionRequirements> handler_mock;
        Fake(fakeit_Method(handler_mock, OnNoteOn));

        engine.SetKeyActionHandler(&handler_mock.get());

        ctx.sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);

        ctx.sensor.last_hammer_speed_m_per_s = +0.5f;
        engine.Execute(0.31f, ctx);

        Verify(fakeit_Method(handler_mock, OnNoteOn)).Never();
        REQUIRE(ctx.sensor.is_note_on == false);
      }
    }

    SECTION("When re-armed after a strike") {
      SECTION("Should allow a second strike without a NoteOff") {
        TestContext ctx{};
        DefaultEngine engine{};
        Mock<midismith::piano_sensing::KeyActionRequirements> handler_mock;
        Fake(fakeit_Method(handler_mock, OnNoteOn));

        engine.SetKeyActionHandler(&handler_mock.get());

        ctx.sensor.last_hammer_speed_m_per_s = -1.0f;

        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        Verify(fakeit_Method(handler_mock, OnNoteOn)).Once();
        REQUIRE(ctx.sensor.is_note_on == true);

        ctx.sensor.last_hammer_speed_m_per_s = +0.5f;
        engine.Execute(0.41f, ctx);

        ctx.sensor.last_hammer_speed_m_per_s = -1.0f;
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        Verify(fakeit_Method(handler_mock, OnNoteOn)).Exactly(2);
        REQUIRE(ctx.sensor.is_note_on == true);
        REQUIRE(ctx.sensor.last_midi_velocity == static_cast<std::uint8_t>(64u));
      }
    }

    SECTION("When configured with a custom mapper type") {
      SECTION("Should use the custom mapper output to emit NoteOn") {
        TestContext ctx{};
        CustomMapperEngine engine{};
        Mock<midismith::piano_sensing::KeyActionRequirements> handler_mock;
        Fake(fakeit_Method(handler_mock, OnNoteOn));

        engine.SetKeyActionHandler(&handler_mock.get());
        RecordingVelocityMapper::Reset();

        ctx.sensor.last_hammer_speed_m_per_s = -1.0f;
        engine.Execute(1.0f, ctx);
        engine.Execute(0.49f, ctx);
        engine.Execute(0.19f, ctx);
        engine.Execute(0.09f, ctx);

        Verify(fakeit_Method(handler_mock, OnNoteOn).Using(static_cast<midismith::midi::Velocity>(23u)))
            .Once();
        REQUIRE(RecordingVelocityMapper::map_calls == 1);
        REQUIRE(RecordingVelocityMapper::last_speed_m_per_s == 1.0f);
      }
    }
  }
}

#endif
