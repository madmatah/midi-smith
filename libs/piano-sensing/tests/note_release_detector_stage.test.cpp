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

TEST_CASE("The NoteReleaseDetectorStage class", "[piano-sensing]") {
  using Catch::Matchers::WithinAbs;
  using Constant127Mapper = midismith::piano_sensing::velocity::ConstantVelocityMapper<127u>;
  using DefaultStage = midismith::piano_sensing::NoteReleaseDetectorStage<Constant127Mapper, 0.5f>;
  using CustomMapperStage =
      midismith::piano_sensing::NoteReleaseDetectorStage<RecordingVelocityMapper, 0.5f>;

  SECTION("The Execute() method") {
    SECTION("On first execution") {
      SECTION("When note is already on and below threshold") {
        SECTION("Should latch current speed if positive") {
          TestContext ctx{};
          CustomMapperStage stage{};
          RecordingKeyActionHandler handler{};
          stage.SetKeyActionHandler(&handler);
          RecordingVelocityMapper::Reset();

          ctx.sensor.is_note_on = true;
          ctx.sensor.last_shank_falling_speed_m_per_s = 0.5f;
          ctx.sensor.last_shank_position_smoothed_norm = 0.40f;

          stage.Execute(0.0f, ctx);

          ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
          stage.Execute(0.0f, ctx);

          REQUIRE(handler.note_off_calls == 1);
          REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                       WithinAbs(0.5f, 0.0001f));
        }
      }

      SECTION("When note is off") {
        SECTION("Should not latch speed") {
          TestContext ctx{};
          CustomMapperStage stage{};
          RecordingKeyActionHandler handler{};
          stage.SetKeyActionHandler(&handler);
          RecordingVelocityMapper::Reset();

          ctx.sensor.is_note_on = false;
          ctx.sensor.last_shank_falling_speed_m_per_s = 0.5f;
          ctx.sensor.last_shank_position_smoothed_norm = 0.40f;

          stage.Execute(0.0f, ctx);

          ctx.sensor.is_note_on = true;
          ctx.sensor.last_shank_position_smoothed_norm = 0.50f;
          stage.Execute(0.0f, ctx);

          ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
          stage.Execute(0.0f, ctx);

          REQUIRE(handler.note_off_calls == 1);
          REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                       WithinAbs(0.0f, 0.0001f));
        }
      }
    }

    SECTION("During steady state") {
      SECTION("When speed is negative or zero") {
        SECTION("Should not update latched speed") {
          TestContext ctx{};
          CustomMapperStage stage{};
          RecordingKeyActionHandler handler{};
          stage.SetKeyActionHandler(&handler);
          RecordingVelocityMapper::Reset();

          ctx.sensor.is_note_on = true;
          ctx.sensor.last_shank_position_smoothed_norm = 0.40f;
          stage.Execute(0.0f, ctx);

          ctx.sensor.last_shank_falling_speed_m_per_s = 0.33f;
          stage.Execute(0.0f, ctx);

          ctx.sensor.last_shank_falling_speed_m_per_s = -1.0f;
          stage.Execute(0.0f, ctx);

          ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
          stage.Execute(0.0f, ctx);

          REQUIRE(handler.note_off_calls == 1);
          REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                       WithinAbs(0.33f, 0.0001f));
        }
      }

      SECTION("When position crosses threshold upwards") {
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

          ctx.sensor.last_shank_position_smoothed_norm = 0.51f;
          stage.Execute(0.0f, ctx);

          REQUIRE(handler.note_off_calls == 1);
          REQUIRE(ctx.sensor.is_note_on == false);
        }
      }
    }

    SECTION("On note-on transition") {
      SECTION("Should reset latched speed") {
        TestContext ctx{};
        CustomMapperStage stage{};
        RecordingKeyActionHandler handler{};
        stage.SetKeyActionHandler(&handler);
        RecordingVelocityMapper::Reset();

        ctx.sensor.is_note_on = false;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.5f;
        stage.Execute(0.0f, ctx);

        stage.Execute(0.0f, ctx);

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_falling_speed_m_per_s = 0.0f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
        stage.Execute(0.0f, ctx);

        REQUIRE(handler.note_off_calls == 1);
        REQUIRE_THAT(RecordingVelocityMapper::last_shank_falling_speed_m_per_s,
                     WithinAbs(0.0f, 0.0001f));
      }
    }
  }

  SECTION("The SetKeyActionHandler() method") {
    SECTION("When given nullptr") {
      SECTION("Should use the null handler and not crash") {
        TestContext ctx{};
        DefaultStage stage{};
        stage.SetKeyActionHandler(nullptr);

        ctx.sensor.is_note_on = true;
        ctx.sensor.last_shank_position_smoothed_norm = 0.40f;
        stage.Execute(0.0f, ctx);

        ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
        stage.Execute(0.0f, ctx);

        REQUIRE(ctx.sensor.is_note_on == false);
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("Should clear internal state") {
      TestContext ctx{};
      CustomMapperStage stage{};
      RecordingKeyActionHandler handler{};
      stage.SetKeyActionHandler(&handler);
      RecordingVelocityMapper::Reset();

      ctx.sensor.is_note_on = true;
      ctx.sensor.last_shank_falling_speed_m_per_s = 0.5f;
      ctx.sensor.last_shank_position_smoothed_norm = 0.40f;
      stage.Execute(0.0f, ctx);

      stage.Reset();

      ctx.sensor.last_shank_position_smoothed_norm = 0.60f;
      stage.Execute(0.0f, ctx);

      REQUIRE(handler.note_off_calls == 0);
    }
  }
}

#endif
