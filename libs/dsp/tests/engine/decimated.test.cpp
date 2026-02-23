#if defined(UNIT_TESTS)

#include "dsp/engine/decimated.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "test_stubs.hpp"

TEST_CASE("The Decimated decorator") {
  using Catch::Matchers::WithinAbs;
  using midismith::dsp::engine::Decimated;
  using midismith::dsp::engine::test::CounterStage;
  using midismith::dsp::engine::test::TestContext;

  SECTION("When configured with Factor=3 and a counter stage") {
    CounterStage::ResetCounts();
    Decimated<CounterStage, 3> pipeline;
    TestContext ctx{};

    SECTION("Should only execute the processing logic every 3 samples") {
      float out1 = pipeline.Transform(10.0f, ctx);
      REQUIRE_THAT(out1, WithinAbs(11.0f, 0.001f));

      float out2 = pipeline.Transform(20.0f, ctx);
      REQUIRE_THAT(out2, WithinAbs(11.0f, 0.001f));

      float out3 = pipeline.Transform(30.0f, ctx);
      REQUIRE_THAT(out3, WithinAbs(11.0f, 0.001f));

      float out4 = pipeline.Transform(40.0f, ctx);
      REQUIRE_THAT(out4, WithinAbs(41.0f, 0.001f));
    }

    SECTION("Should verify CPU savings via counters") {
      REQUIRE(CounterStage::push_count == 0);
      REQUIRE(CounterStage::compute_count == 0);
      pipeline.Transform(1.0f, ctx);
      REQUIRE(CounterStage::push_count == 1);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Transform(2.0f, ctx);
      REQUIRE(CounterStage::push_count == 2);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Transform(3.0f, ctx);
      REQUIRE(CounterStage::push_count == 3);
      REQUIRE(CounterStage::compute_count == 1);
      pipeline.Transform(4.0f, ctx);
      REQUIRE(CounterStage::push_count == 4);
      REQUIRE(CounterStage::compute_count == 2);
    }
  }
}

#endif
