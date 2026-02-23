#if defined(UNIT_TESTS)

#include "piano-sensing/note_release_detector_stage.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

class RecordingKeyActionHandler final : public midismith::piano_sensing::KeyActionRequirements {
 public:
  void OnNoteOn(midismith::midi::Velocity velocity) noexcept override {
    (void) velocity;
  }

  void OnNoteOff(midismith::midi::Velocity release_velocity) noexcept override {
    ++note_off_calls;
    last_release_velocity = release_velocity;
  }

  int note_off_calls = 0;
  midismith::midi::Velocity last_release_velocity = 0u;
};

class RecordingVelocityMapper final
    : public midismith::piano_sensing::velocity::VelocityMapperRequirements {
 public:
  static void Reset() noexcept {
    map_calls = 0;
    last_shank_falling_speed_m_per_s = 0.0f;
  }

  midismith::midi::Velocity Map(float speed_m_per_s) noexcept override {
    last_shank_falling_speed_m_per_s = speed_m_per_s;
    ++map_calls;
    return (speed_m_per_s > 0.0f) ? static_cast<midismith::midi::Velocity>(42u)
                                  : static_cast<midismith::midi::Velocity>(1u);
  }

  static inline int map_calls = 0;
  static inline float last_shank_falling_speed_m_per_s = 0.0f;
};

struct TestContext {
  struct Sensor {
    float last_shank_position_smoothed_norm = 0.0f;
    float last_shank_falling_speed_m_per_s = 0.0f;
    bool is_note_on = false;
  } sensor;
};

}  // namespace

TEST_CASE("The NoteReleaseDetectorStage class") {
  using Catch::Matchers::WithinAbs;
  using Constant127Mapper = midismith::piano_sensing::velocity::ConstantVelocityMapper<127u>;
  using DefaultStage = midismith::piano_sensing::NoteReleaseDetectorStage<Constant127Mapper, 0.5f>;
  using CustomMapperStage =
      midismith::piano_sensing::NoteReleaseDetectorStage<RecordingVelocityMapper, 0.5f>;

  SECTION("The note release detection") {
    SECTION("When note is on and position crosses the threshold upwards") {
      SECTION("Should emit NoteOff and clear is_note_on") {
        TestContext ctx{};
        CustomMapperStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.25f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.49f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_falling_speed_m_per_s = 0.0f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                     WithinAbs(0.25f, 0.0001f));
        REQUIRE(handler.last_release_velocity == static_cast<midismith::midi::Velocity>(42u));
        REQUIRE(ctx.sensor.is_note_on == false);
      }
    }

    SECTION("When speed becomes gated at the threshold") {
      SECTION("Should map the last latched positive speed") {
        TestContext ctx{};
        CustomMapperStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.10f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.40f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_falling_speed_m_per_s = 0.35f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.45f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_falling_speed_m_per_s = 0.0f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                     WithinAbs(0.35f, 0.0001f));
      }
    }

    SECTION("When note is off") {
      SECTION("Should not emit NoteOff") {
        TestContext ctx{};
        CustomMapperStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        ctx.sensor.is_note_on = false;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.25f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.49f;
        stage.Execute(0.0f, ctx);
        ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 0);
        REQUIRE(ctx.sensor.is_note_on == false);
      }
    }

    SECTION("When no custom velocity mapper is configured") {
      SECTION("Should emit NoteOff with default velocity 127") {
        TestContext ctx{};
        DefaultStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.25f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.49f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_falling_speed_m_per_s = 0.0f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE(handler.last_release_velocity == static_cast<midismith::midi::Velocity>(127u));
        REQUIRE(ctx.sensor.is_note_on == false);
      }
    }

    SECTION("When configured with a custom mapper type") {
      SECTION("Should use the custom mapper output to emit NoteOff") {
        TestContext ctx{};
        CustomMapperStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.25f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.49f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_falling_speed_m_per_s = 0.0f;
        ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE(handler.last_release_velocity == static_cast<midismith::midi::Velocity>(42u));
        REQUIRE(RecordingVelocityMapper::map_calls == 1);
        REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                     WithinAbs(0.25f, 0.0001f));
      }
    }
  }
}

#endif
