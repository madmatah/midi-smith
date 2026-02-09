#if defined(UNIT_TESTS)

#include "domain/dsp/engine/workflow.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "test_stubs.hpp"

TEST_CASE("The Workflow class") {
  using Catch::Matchers::WithinAbs;
  using domain::dsp::engine::Workflow;
  using domain::dsp::engine::test::ConsumerStage;
  using domain::dsp::engine::test::CounterStage;
  using domain::dsp::engine::test::PlusTenStage;
  using domain::dsp::engine::test::TestContext;
  using domain::dsp::engine::test::TimesTwoStage;

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
      Workflow<PlusTenStage, ConsumerStage, TimesTwoStage> pipeline;
      TestContext ctx{};

      SECTION("Should execute the consumer and keep value propagation") {
        REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
        REQUIRE(ctx.consume_count == 1);
        REQUIRE_THAT(ctx.last_value, WithinAbs(15.0f, 0.001f));
      }
    }
  }

  SECTION("The Execute() method") {
    Workflow<PlusTenStage, ConsumerStage> pipeline;
    TestContext ctx{};

    SECTION("Should run the stages and ignore the output") {
      pipeline.Execute(5.0f, ctx);
      REQUIRE(ctx.consume_count == 1);
      REQUIRE_THAT(ctx.last_value, WithinAbs(15.0f, 0.001f));
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
}

#endif
