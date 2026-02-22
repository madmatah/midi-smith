#if defined(UNIT_TESTS)

#include "domain/dsp/logic/input_gate.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {

struct MockContext {
  float control_value;
};

float GetControlValue(const MockContext& ctx) {
  return ctx.control_value;
}

}  // namespace

TEST_CASE("The InputGate class") {
  using Catch::Matchers::WithinAbs;
  using midismith::adc_board::domain::dsp::logic::InputGate;

  SECTION("The Transform() method") {
    SECTION("When control value is below threshold") {
      SECTION("Should return the sample unchanged") {
        InputGate<0.5f, GetControlValue> gate;
        MockContext ctx{.control_value = 0.4f};

        const float result = gate.Transform(1.23f, ctx);

        REQUIRE_THAT(result, WithinAbs(1.23f, 0.001f));
      }
    }

    SECTION("When control value is equal to threshold") {
      SECTION("Should return 0.0f") {
        InputGate<0.5f, GetControlValue> gate;
        MockContext ctx{.control_value = 0.5f};

        const float result = gate.Transform(1.23f, ctx);

        REQUIRE_THAT(result, WithinAbs(0.0f, 0.001f));
      }
    }

    SECTION("When control value is above threshold") {
      SECTION("Should return 0.0f") {
        InputGate<0.5f, GetControlValue> gate;
        MockContext ctx{.control_value = 0.6f};

        const float result = gate.Transform(1.23f, ctx);

        REQUIRE_THAT(result, WithinAbs(0.0f, 0.001f));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("Should be callable without side effects") {
      InputGate<0.5f, GetControlValue> gate;

      gate.Reset();

      REQUIRE(true);
    }
  }
}

#endif
