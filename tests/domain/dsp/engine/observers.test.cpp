#if defined(UNIT_TESTS)

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstdint>

#include "domain/dsp/engine/capture.hpp"
#include "domain/dsp/engine/decimated.hpp"
#include "domain/dsp/engine/tap.hpp"
#include "domain/dsp/engine/workflow.hpp"

namespace {

struct ObserverContext {
  struct Nested {
    float value = 0.0f;
  };

  std::uint32_t consume_count = 0;
  float last_value = 0.0f;
  float captured = 0.0f;
  float temp_c = 0.0f;
  Nested sensor{};
};

class PlusTenStage {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample + 10.0f;
  }
};

class TimesTwoStage {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample * 2.0f;
  }
};

class PlusHundredStage {
 public:
  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    return sample + 100.0f;
  }
};

class ConsumerStage {
 public:
  template <typename ContextT>
  void Execute(float sample, ContextT& ctx) noexcept {
    ctx.consume_count++;
    ctx.last_value = sample;
  }
};

class CountingTransformer {
 public:
  void Reset() noexcept {
    transform_count = 0;
  }

  template <typename ContextT>
  float Transform(float sample, const ContextT& ctx) noexcept {
    (void) ctx;
    transform_count++;
    return sample + 123.0f;
  }

  static inline std::uint32_t transform_count = 0;
};

class ResetTrackingStage {
 public:
  void Reset() noexcept {
    reset_count++;
  }

  template <typename ContextT>
  void Execute(float sample, ContextT& ctx) noexcept {
    (void) sample;
    (void) ctx;
  }

  static inline std::uint32_t reset_count = 0;
};

using domain::dsp::engine::Capture;
using domain::dsp::engine::Decimated;
using domain::dsp::engine::Tap;
using domain::dsp::engine::Workflow;

using PureWorkflow = Workflow<PlusTenStage, TimesTwoStage>;
static_assert(domain::dsp::concepts::SignalTransformer<PureWorkflow, ObserverContext>);

using CaptureWorkflow = Workflow<PlusTenStage, Capture<&ObserverContext::captured>, TimesTwoStage>;
static_assert(domain::dsp::concepts::SignalTransformer<CaptureWorkflow, ObserverContext>);

using TapWorkflow = Workflow<PlusTenStage, Tap<Workflow<TimesTwoStage>>, TimesTwoStage>;
static_assert(domain::dsp::concepts::SignalTransformer<TapWorkflow, ObserverContext>);

}  // namespace

TEST_CASE("Capture and Tap observers") {
  using Catch::Matchers::WithinAbs;

  SECTION("Capture copies the input into the context without altering the flow") {
    Workflow<PlusTenStage, Capture<&ObserverContext::captured>, TimesTwoStage> pipeline;
    ObserverContext ctx{};

    REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
    REQUIRE_THAT(ctx.captured, WithinAbs(15.0f, 0.001f));
  }

  SECTION("Capture can use a stateless lambda to write into nested structures") {
    Workflow<PlusTenStage, Capture<[](ObserverContext& ctx, float v) { ctx.sensor.value = v; }>,
             TimesTwoStage>
        pipeline;
    ObserverContext ctx{};

    REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
    REQUIRE_THAT(ctx.sensor.value, WithinAbs(15.0f, 0.001f));
  }

  SECTION("Tap executes a transformer content and ignores its output") {
    CountingTransformer::transform_count = 0;
    Workflow<PlusTenStage, Tap<CountingTransformer>, TimesTwoStage> pipeline;
    ObserverContext ctx{};

    auto& tap = pipeline.Stage<1>();
    (void) tap.Content();

    REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
    REQUIRE(CountingTransformer::transform_count == 1);
  }

  SECTION("Tap executes a consumer content and keeps value propagation") {
    Workflow<PlusTenStage, Tap<ConsumerStage>, TimesTwoStage> pipeline;
    ObserverContext ctx{};

    REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
    REQUIRE(ctx.consume_count == 1);
    REQUIRE_THAT(ctx.last_value, WithinAbs(15.0f, 0.001f));
  }

  SECTION("Tap can wrap a Workflow to compute derived values") {
    using Inner = Workflow<PlusHundredStage, Capture<&ObserverContext::temp_c>>;
    Workflow<PlusTenStage, Tap<Inner>, TimesTwoStage> pipeline;
    ObserverContext ctx{};

    REQUIRE_THAT(pipeline.Transform(5.0f, ctx), WithinAbs(30.0f, 0.001f));
    REQUIRE_THAT(ctx.temp_c, WithinAbs(115.0f, 0.001f));
  }

  SECTION("Decimated can wrap Tap to reduce capture frequency") {
    Workflow<PlusTenStage, Decimated<Tap<ConsumerStage>, 3>, TimesTwoStage> pipeline;
    ObserverContext ctx{};

    REQUIRE_THAT(pipeline.Transform(1.0f, ctx), WithinAbs(22.0f, 0.001f));
    REQUIRE(ctx.consume_count == 1);
    REQUIRE_THAT(ctx.last_value, WithinAbs(11.0f, 0.001f));

    REQUIRE_THAT(pipeline.Transform(2.0f, ctx), WithinAbs(24.0f, 0.001f));
    REQUIRE_THAT(pipeline.Transform(3.0f, ctx), WithinAbs(26.0f, 0.001f));
    REQUIRE(ctx.consume_count == 1);
    REQUIRE_THAT(ctx.last_value, WithinAbs(11.0f, 0.001f));

    REQUIRE_THAT(pipeline.Transform(4.0f, ctx), WithinAbs(28.0f, 0.001f));
    REQUIRE(ctx.consume_count == 2);
    REQUIRE_THAT(ctx.last_value, WithinAbs(14.0f, 0.001f));
  }

  SECTION("Reset propagates to Tap content when content is Resettable") {
    ResetTrackingStage::reset_count = 0;
    Workflow<Tap<ResetTrackingStage>> pipeline;
    pipeline.Reset();
    REQUIRE(ResetTrackingStage::reset_count == 1);
  }
}

#endif
