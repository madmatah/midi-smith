#if defined(UNIT_TESTS)

#include "app/shell/commands/calibration_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string>

#include "app/calibration/calibration_query_requirements.hpp"
#include "app/config/sensors.hpp"
#include "calibration/board_calibration_data.hpp"
#include "calibration/sensor_calibration.hpp"
#include "io/stream_requirements.hpp"

namespace {

class RecordingStream : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }
  void Write(const char* str) noexcept override {
    if (str != nullptr) {
      output_ += str;
    }
  }
  const std::string& output() const {
    return output_;
  }

 private:
  std::string output_;
};

class CalibrationQueryStub
    : public midismith::adc_board::app::calibration::CalibrationQueryRequirements {
 public:
  const midismith::calibration::SensorCalibration& sensor_calibration(
      std::uint8_t sensor_index) const noexcept override {
    return data_[sensor_index];
  }

  midismith::calibration::BoardCalibrationData<
      midismith::adc_board::app::config::sensors::kSensorCount>
      data_{};
};

}  // namespace

TEST_CASE("The CalibrationCommand class") {
  CalibrationQueryStub query_stub;
  midismith::adc_board::app::shell::commands::CalibrationCommand command(query_stub);
  RecordingStream out;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'calibration'") {
        REQUIRE(command.Name() == "calibration");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return a non-empty description") {
        REQUIRE_FALSE(command.Help().empty());
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called without arguments") {
      SECTION("Should print usage") {
        char* argv[] = {nullptr};
        command.Run(0, argv, out);
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("usage:"));
      }
    }

    SECTION("When called with an unknown subcommand") {
      SECTION("Should print usage") {
        char arg0[] = "calibration";
        char arg1[] = "unknown";
        char* argv[] = {arg0, arg1};
        command.Run(2, argv, out);
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("usage:"));
      }
    }

    SECTION("When called with 'status'") {
      query_stub.data_[0] = midismith::calibration::SensorCalibration{
          .rest_current_ma = 0.127f,
          .strike_current_ma = 0.642f,
          .rest_distance_mm = 7.0f,
          .strike_distance_mm = 1.9f,
      };

      char arg0[] = "calibration";
      char arg1[] = "status";
      char* argv[] = {arg0, arg1};
      command.Run(2, argv, out);

      SECTION("Should print one line per sensor") {
        std::size_t line_count = 0;
        for (char c : out.output()) {
          if (c == '\n') {
            ++line_count;
          }
        }
        REQUIRE(line_count == midismith::adc_board::app::config::sensors::kSensorCount);
      }

      SECTION("Should include sensor[1] in the output") {
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("sensor[1]"));
      }

      SECTION("Should include rest_mA label in the output") {
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("rest_mA="));
      }

      SECTION("Should include strike_mA label in the output") {
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("strike_mA="));
      }

      SECTION("Should include rest_mm label in the output") {
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("rest_mm="));
      }

      SECTION("Should include strike_mm label in the output") {
        REQUIRE_THAT(out.output(), Catch::Matchers::ContainsSubstring("strike_mm="));
      }
    }
  }
}

#endif
