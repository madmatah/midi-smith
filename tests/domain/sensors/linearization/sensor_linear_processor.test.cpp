#if defined(UNIT_TESTS)

#include "domain/sensors/linearization/sensor_linear_processor.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <cstddef>

#include "domain/sensors/linearization/cny70_response_curve.hpp"
#include "domain/sensors/linearization/lookup_table_generator.hpp"

namespace {

struct TestContext {};

}  // namespace

TEST_CASE("The SensorLinearProcessor class") {
  using Catch::Matchers::WithinAbs;
  using domain::sensors::linearization::SensorLinearProcessor;
  using domain::sensors::linearization::SensorLookupTable;

  constexpr std::size_t kLookupTableSize = 256u;
  SensorLinearProcessor<kLookupTableSize> processor;

  SECTION("Transform() works with arbitrary scale and bias") {
    SensorLookupTable<kLookupTableSize> table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
      table[i] = static_cast<float>(i) / static_cast<float>(table.size() - 1u);
    }

    SensorLinearProcessor<kLookupTableSize>::Configuration configuration{
        .lookup_table = &table,
        .input_to_lut_index_scale = static_cast<float>(kLookupTableSize - 1u),
        .input_to_lut_index_bias = 0.0f,
    };
    processor.ApplyConfiguration(&configuration);

    TestContext ctx{};
    REQUIRE_THAT(processor.Transform(0.0f, ctx), WithinAbs(0.0f, 0.0001f));
    REQUIRE_THAT(processor.Transform(0.5f, ctx), WithinAbs(0.5f, 0.01f));
    REQUIRE_THAT(processor.Transform(1.0f, ctx), WithinAbs(1.0f, 0.0001f));
  }

  SECTION("Transform() supports negative scale") {
    SensorLookupTable<kLookupTableSize> table{};
    for (std::size_t i = 0; i < table.size(); ++i) {
      table[i] = static_cast<float>(i) / static_cast<float>(table.size() - 1u);
    }

    SensorLinearProcessor<kLookupTableSize>::Configuration configuration{
        .lookup_table = &table,
        .input_to_lut_index_scale = -static_cast<float>(kLookupTableSize - 1u),
        .input_to_lut_index_bias = static_cast<float>(kLookupTableSize - 1u),
    };
    processor.ApplyConfiguration(&configuration);

    TestContext ctx{};
    REQUIRE_THAT(processor.Transform(0.0f, ctx), WithinAbs(1.0f, 0.0001f));
    REQUIRE_THAT(processor.Transform(0.25f, ctx), WithinAbs(0.75f, 0.01f));
    REQUIRE_THAT(processor.Transform(1.0f, ctx), WithinAbs(0.0f, 0.0001f));
  }

  SECTION("ApplyConfiguration() switches the output immediately") {
    SensorLookupTable<kLookupTableSize> table_a{};
    for (auto& x : table_a) {
      x = 0.900f;
    }
    SensorLookupTable<kLookupTableSize> table_b{};
    for (auto& x : table_b) {
      x = 0.123f;
    }

    SensorLinearProcessor<kLookupTableSize>::Configuration configuration_a{
        .lookup_table = &table_a,
        .input_to_lut_index_scale = 1.0f,
        .input_to_lut_index_bias = 0.0f,
    };
    processor.ApplyConfiguration(&configuration_a);

    SensorLinearProcessor<kLookupTableSize>::Configuration configuration_b{
        .lookup_table = &table_b,
        .input_to_lut_index_scale = 1.0f,
        .input_to_lut_index_bias = 0.0f,
    };
    processor.ApplyConfiguration(&configuration_b);

    TestContext ctx{};
    REQUIRE_THAT(processor.Transform(0.50f, ctx), WithinAbs(0.123f, 0.0001f));
  }

  SECTION("CNY70 generator + processor maps strike to 0 and rest to 1") {
    const auto master = domain::sensors::linearization::Cny70DatasheetSensorResponseCurve();
    const domain::sensors::linearization::SensorCalibration calibration{
        .rest_current_ma = 0.047f,
        .strike_current_ma = 1.000f,
        .rest_distance_mm = 10.0f,
        .strike_distance_mm = 0.5f,
    };

    SensorLookupTable<kLookupTableSize> lookup_table{};
    const auto result = domain::sensors::linearization::LookupTableGenerator::Generate(
        master, calibration, lookup_table);

    SensorLinearProcessor<kLookupTableSize>::Configuration configuration = result.configuration;
    processor.ApplyConfiguration(&configuration);

    TestContext ctx{};
    REQUIRE_THAT(processor.Transform(calibration.strike_current_ma, ctx), WithinAbs(0.0f, 0.01f));
    REQUIRE_THAT(processor.Transform(calibration.rest_current_ma, ctx), WithinAbs(1.0f, 0.01f));
  }
}

#endif
