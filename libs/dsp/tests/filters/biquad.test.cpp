#if defined(UNIT_TESTS)

#include "dsp/filters/biquad.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cmath>
#include <cstdint>

#include "dsp/concepts.hpp"

namespace {

struct TestContext {};

struct SignInversionStrategy {
  midismith::dsp::filters::BiquadCoefficients Coefficients() const noexcept {
    midismith::dsp::filters::BiquadCoefficients c{};
    c.b0 = 1.0f;
    c.b1 = 0.0f;
    c.b2 = 0.0f;
    c.a1 = 0.5f;
    c.a2 = 0.0f;
    return c;
  }
};

using FilterWithSignInversion = midismith::dsp::filters::Biquad<SignInversionStrategy>;
static_assert(midismith::dsp::concepts::SignalTransformer<FilterWithSignInversion, TestContext>);
static_assert(midismith::dsp::concepts::Resettable<FilterWithSignInversion>);

}  // namespace

TEST_CASE("The Biquad class") {
  using Catch::Matchers::WithinAbs;

  SECTION("The Transform() method") {
    SECTION("When the strategy provides stable cookbook coefficients") {
      SECTION("Should apply the CMSIS sign convention for a1/a2") {
        FilterWithSignInversion filter;
        TestContext ctx{};

        REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinAbs(1.0f, 0.0001f));
        REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinAbs(0.5f, 0.0001f));
        REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinAbs(0.75f, 0.0001f));
        REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinAbs(0.625f, 0.0001f));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after processing samples") {
      SECTION("Should restore the initial state") {
        FilterWithSignInversion filter;
        TestContext ctx{};

        float initial_outputs[6]{};
        for (float& v : initial_outputs) {
          v = filter.Transform(1.0f, ctx);
        }

        filter.Reset();

        for (const float expected : initial_outputs) {
          REQUIRE_THAT(filter.Transform(1.0f, ctx), WithinAbs(expected, 0.0001f));
        }
      }
    }
  }

  SECTION("The LowPassStrategy helper") {
    SECTION("When used with a constant signal") {
      SECTION("Should remain finite and converge near DC gain 1") {
        using LowPass = midismith::dsp::filters::LowPassStrategy<48000u, 1000u, 0.707f>;
        midismith::dsp::filters::Biquad<LowPass> filter;
        TestContext ctx{};

        float y = 0.0f;
        for (std::uint32_t i = 0; i < 200u; ++i) {
          y = filter.Transform(1.0f, ctx);
          REQUIRE(std::isfinite(y));
        }

        REQUIRE_THAT(y, WithinAbs(1.0f, 0.05f));
      }
    }
  }

  SECTION("The NotchStrategy helper") {
    SECTION("When used with a sine at cutoff frequency") {
      SECTION("Should strongly attenuate the signal") {
        constexpr std::uint32_t kSampleRateHz = 48000u;
        constexpr std::uint32_t kCutoffHz = 1000u;
        constexpr std::uint32_t kSamplesPerPeriod = kSampleRateHz / kCutoffHz;
        static_assert(kSamplesPerPeriod > 0u);

        using Notch = midismith::dsp::filters::NotchStrategy<kSampleRateHz,
                                                             static_cast<float>(kCutoffHz), 5.0f>;
        midismith::dsp::filters::Biquad<Notch> filter;
        TestContext ctx{};

        constexpr std::uint32_t kWarmupPeriods = 20u;
        constexpr std::uint32_t kMeasurePeriods = 40u;

        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kOmega =
            2.0f * kPi * static_cast<float>(kCutoffHz) / static_cast<float>(kSampleRateHz);

        for (std::uint32_t n = 0u; n < (kWarmupPeriods * kSamplesPerPeriod); ++n) {
          const float x = std::sin(kOmega * static_cast<float>(n));
          (void) filter.Transform(x, ctx);
        }

        double sum_in2 = 0.0;
        double sum_out2 = 0.0;
        const std::uint32_t kMeasureSamples = kMeasurePeriods * kSamplesPerPeriod;
        for (std::uint32_t i = 0u; i < kMeasureSamples; ++i) {
          const std::uint32_t n = (kWarmupPeriods * kSamplesPerPeriod) + i;
          const float x = std::sin(kOmega * static_cast<float>(n));
          const float y = filter.Transform(x, ctx);
          sum_in2 += static_cast<double>(x) * static_cast<double>(x);
          sum_out2 += static_cast<double>(y) * static_cast<double>(y);
        }

        const float input_rms = std::sqrt(static_cast<float>(sum_in2 / kMeasureSamples));
        const float output_rms = std::sqrt(static_cast<float>(sum_out2 / kMeasureSamples));
        REQUIRE(input_rms > 0.0f);

        const float gain = output_rms / input_rms;
        REQUIRE(std::isfinite(gain));
        REQUIRE(gain < 0.02f);
      }
    }
  }
}

#endif
