#if defined(UNIT_TESTS)

#include "domain/dsp/engine/workflow.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "domain/dsp/engine/tap.hpp"
#include "test_stubs.hpp"

TEST_CASE("The Workflow class") {
  using Catch::Matchers::WithinAbs;
  using midismith::adc_board::domain::dsp::engine::Tap;
  using midismith::adc_board::domain::dsp::engine::Workflow;
  using midismith::adc_board::domain::dsp::engine::test::ConsumerStage;
  using midismith::adc_board::domain::dsp::engine::test::CounterStage;
  using midismith::adc_board::domain::dsp::engine::test::PlusTenStage;
  using midismith::adc_board::domain::dsp::engine::test::TestContext;
  using midismith::adc_board::domain::dsp::engine::test::TimesTwoStage;

  SECTION("The Transform() method") {
    SECTION("When processed with a single stage") {
      Workflow<PlusTenStage> pipeline;
      TestContext ctx{};

      SECTION("Should apply the stage systematically on Transform()") {
        REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(15.0f, 0.001f));
        REQUIRE_THAT(pipeline.Transform(20.0f, ctx), WithinAbs(30.0f, 0.001f));
      }
    }

    SECTION("When processed with multiple stages") {
      Workflow<PlusTenStage, TimesTwoStage> pipeline;
      TestContext ctx{};

      SECTION("Should chain the stages systematically") {
        REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
        REQUIRE_THAT(pipeline.Transform(6.0f, ctx), WithinAbs(32.0f, 0.001f));
      }
    }

    SECTION("When processed with a consumer stage") {
      Workflow<PlusTenStage, Tap<ConsumerStage>, TimesTwoStage> pipeline;
      TestContext ctx{};

      SECTION("Should execute the consumer and keep value propagation") {
        REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
        REQUIRE(ctx.consume_count == 1);
        REQUIRE_THAT(ctx.last_value, WithinAbs(15.0f, 0.001f));
      }
    }
  }

  SECTION("The Reset() method") {
    CounterStage::ResetCounts();
    Workflow<CounterStage> pipeline;
    TestContext ctx{};

    SECTION("Should reset the stages systematically") {
      pipeline.Transform(1.0f, ctx);
      REQUIRE(CounterStage::reset_count == 0);
      pipeline.Reset();
      REQUIRE(CounterStage::reset_count == 1);
    }
  }

  SECTION("The Stage() accessor") {
    Workflow<PlusTenStage, TimesTwoStage> pipeline;

    SECTION("Should return stable references to stages") {
      auto& stage0 = pipeline.Stage<0>();
      auto& stage0_again = pipeline.Stage<0>();
      REQUIRE(&stage0 == &stage0_again);

      const auto& const_pipeline = pipeline;
      const auto& const_stage1 = const_pipeline.Stage<1>();
      (void) const_stage1;
    }
  }

  SECTION("The StageIndexOf trait") {
    using Pipeline = Workflow<PlusTenStage, TimesTwoStage, Tap<ConsumerStage>>;

    SECTION("Should resolve the index of a direct stage by type") {
      STATIC_REQUIRE(Pipeline::StageIndexOf<PlusTenStage> == 0);
      STATIC_REQUIRE(Pipeline::StageIndexOf<TimesTwoStage> == 1);
    }

    SECTION("Should resolve the index of a Tap-wrapped stage by its full type") {
      STATIC_REQUIRE(Pipeline::StageIndexOf<Tap<ConsumerStage>> == 2);
    }

    SECTION("Should allow Stage() access via StageIndexOf") {
      Pipeline pipeline;
      auto& stage = pipeline.Stage<Pipeline::StageIndexOf<PlusTenStage>>();
      auto& same_stage = pipeline.Stage<0>();
      REQUIRE(&stage == &same_stage);
    }
  }
}

#endif
