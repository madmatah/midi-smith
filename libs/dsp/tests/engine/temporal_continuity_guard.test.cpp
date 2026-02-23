#if defined(UNIT_TESTS)

#include "dsp/engine/temporal_continuity_guard.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

#include "dsp/filters/constant_filter.hpp"
#include "dsp/logic/switch.hpp"

namespace {

struct TestContext {
  std::uint32_t timestamp_ticks = 0u;
  bool is_active = true;
};

struct IsActive {
  static bool Test(const TestContext& ctx) noexcept {
    return ctx.is_active;
  }
};

class OffsetStage {
 public:
  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    ++transform_count_;
    return sample + 1.0f;
  }

  void Reset() noexcept {
    ++reset_count_;
  }

  std::uint32_t reset_count() const noexcept {
    return reset_count_;
  }

  std::uint32_t transform_count() const noexcept {
    return transform_count_;
  }

 private:
  std::uint32_t reset_count_ = 0u;
  std::uint32_t transform_count_ = 0u;
};

class BackwardLikeStage {
 public:
  float Transform(float sample, const TestContext& ctx) noexcept {
    (void) ctx;
    if (!has_last_sample_) {
      last_sample_ = sample;
      has_last_sample_ = true;
      return 0.0f;
    }

    const float speed = sample - last_sample_;
    last_sample_ = sample;
    return speed;
  }

  void Reset() noexcept {
    ++reset_count_;
    last_sample_ = 0.0f;
    has_last_sample_ = false;
  }

  std::uint32_t reset_count() const noexcept {
    return reset_count_;
  }

 private:
  float last_sample_ = 0.0f;
  bool has_last_sample_ = false;
  std::uint32_t reset_count_ = 0u;
};

}  // namespace

TEST_CASE("The TemporalContinuityGuard class") {
  using Catch::Matchers::WithinAbs;
  using midismith::dsp::engine::TemporalContinuityGuard;

  SECTION("The Transform() method") {
    SECTION("On the first call") {
      SECTION("Should pass through without forcing a reset") {
        TemporalContinuityGuard<OffsetStage, 5u> guard;
        TestContext ctx{.timestamp_ticks = 1000u, .is_active = true};

        const float output = guard.Transform(2.0f, ctx);

        REQUIRE_THAT(output, WithinAbs(3.0f, 0.001f));
        REQUIRE(guard.Stage().reset_count() == 0u);
        REQUIRE(guard.Stage().transform_count() == 1u);
      }
    }

    SECTION("When the second sample arrives") {
      SECTION("Should calibrate the nominal delta without resetting the stage") {
        TemporalContinuityGuard<OffsetStage, 5u> guard;
        TestContext ctx{.timestamp_ticks = 1000u, .is_active = true};

        (void) guard.Transform(2.0f, ctx);
        ctx.timestamp_ticks = 1010u;
        const float output = guard.Transform(4.0f, ctx);

        REQUIRE_THAT(output, WithinAbs(5.0f, 0.001f));
        REQUIRE(guard.Stage().reset_count() == 0u);
        REQUIRE(guard.Stage().transform_count() == 2u);
      }
    }

    SECTION("When a temporal gap is detected") {
      SECTION("Should reset the inner stage before processing the current sample") {
        TemporalContinuityGuard<BackwardLikeStage, 5u> guard;
        TestContext ctx{.timestamp_ticks = 1000u, .is_active = true};

        REQUIRE_THAT(guard.Transform(1.0f, ctx), WithinAbs(0.0f, 0.001f));
        ctx.timestamp_ticks = 1010u;
        REQUIRE_THAT(guard.Transform(1.1f, ctx), WithinAbs(0.1f, 0.001f));

        ctx.timestamp_ticks = 1100u;
        const float output = guard.Transform(2.0f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.001f));
        REQUIRE(guard.Stage().reset_count() == 1u);
      }
    }

    SECTION("When timestamp ticks wrap around on 32 bits") {
      SECTION("Should not trigger a false gap when delta is nominal") {
        TemporalContinuityGuard<BackwardLikeStage, 5u> guard;
        TestContext ctx{.timestamp_ticks = 0xFFFFFFF0u, .is_active = true};

        REQUIRE_THAT(guard.Transform(1.0f, ctx), WithinAbs(0.0f, 0.001f));
        ctx.timestamp_ticks = 0xFFFFFFFAu;
        REQUIRE_THAT(guard.Transform(1.1f, ctx), WithinAbs(0.1f, 0.001f));

        ctx.timestamp_ticks = 0x00000004u;
        const float output = guard.Transform(1.2f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.1f, 0.001f));
        REQUIRE(guard.Stage().reset_count() == 0u);
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("After processing samples") {
      SECTION("Should reset both guard state and wrapped stage state") {
        TemporalContinuityGuard<BackwardLikeStage, 5u> guard;
        TestContext ctx{.timestamp_ticks = 1000u, .is_active = true};

        (void) guard.Transform(1.0f, ctx);
        ctx.timestamp_ticks = 1010u;
        (void) guard.Transform(1.2f, ctx);

        guard.Reset();
        ctx.timestamp_ticks = 1020u;
        const float output = guard.Transform(1.3f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.001f));
        REQUIRE(guard.Stage().reset_count() == 1u);
      }
    }
  }

  SECTION("The integration with Switch") {
    SECTION("When the stage is paused by predicate and resumes later") {
      SECTION("Should avoid a resume spike by resetting on the first delayed sample") {
        using GuardedEstimator = TemporalContinuityGuard<BackwardLikeStage, 5u>;
        using SmartEstimator =
            midismith::dsp::logic::Switch<IsActive, GuardedEstimator,
                                          midismith::dsp::filters::ConstantFilter<0.0f>>;
        SmartEstimator stage;
        TestContext ctx{.timestamp_ticks = 1000u, .is_active = true};

        REQUIRE_THAT(stage.Transform(0.2f, ctx), WithinAbs(0.0f, 0.001f));

        ctx.timestamp_ticks = 1010u;
        REQUIRE_THAT(stage.Transform(0.4f, ctx), WithinAbs(0.2f, 0.001f));

        ctx.timestamp_ticks = 1020u;
        ctx.is_active = false;
        REQUIRE_THAT(stage.Transform(0.9f, ctx), WithinAbs(0.0f, 0.001f));

        ctx.timestamp_ticks = 5000u;
        ctx.is_active = true;
        const float output = stage.Transform(1.0f, ctx);

        REQUIRE_THAT(output, WithinAbs(0.0f, 0.001f));
        REQUIRE(stage.TrueStage().Stage().reset_count() == 1u);
      }
    }
  }
}

#endif
