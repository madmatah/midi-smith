#if defined(UNIT_TESTS)
#include "app/shell/commands/sensor_rtt_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "app/config/analog_acquisition.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"
#include "io/stream_requirements.hpp"

namespace {

class StreamStub : public midismith::io::StreamRequirements {
 public:
  midismith::io::ReadResult Read(std::uint8_t&) noexcept override {
    return midismith::io::ReadResult::kNoData;
  }
  void Write(char c) noexcept override {
    output_ += c;
  }
  void Write(const char* str) noexcept override {
    output_ += str;
  }
  void clear() {
    output_.clear();
  }
  const std::string& output() const {
    return output_;
  }

 private:
  std::string output_;
};

class ControlMock
    : public midismith::adc_board::app::telemetry::SensorRttTelemetryControlRequirements {
 public:
  bool RequestOff() noexcept override {
    off_requested_ = true;
    return true;
  }
  bool RequestObserve(std::uint8_t sensor_id) noexcept override {
    last_observe_id_ = sensor_id;
    observe_requested_ = true;
    return true;
  }
  bool RequestSetOutputHz(std::uint32_t output_hz) noexcept override {
    last_output_hz_ = output_hz;
    output_hz_requested_ = true;
    return true;
  }
  midismith::adc_board::app::telemetry::SensorRttTelemetryStatus GetStatus()
      const noexcept override {
    return status_;
  }

  midismith::adc_board::app::telemetry::SensorRttTelemetryStatus status_{};
  bool off_requested_ = false;
  bool observe_requested_ = false;
  bool output_hz_requested_ = false;
  std::uint8_t last_observe_id_ = 0;
  std::uint32_t last_output_hz_ = 0;
};

}  // namespace

TEST_CASE("The SensorRttCommand class", "[app][shell][commands]") {
  midismith::adc_board::domain::sensors::SensorState sensors[] = {
      midismith::adc_board::domain::sensors::SensorState{.id = 1},
      midismith::adc_board::domain::sensors::SensorState{.id = 2}};
  midismith::adc_board::domain::sensors::SensorRegistry registry(sensors, 2);
  ControlMock control;
  midismith::adc_board::app::shell::commands::SensorRttCommand command(registry, control);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'sensor_rtt'") {
        REQUIRE(command.Name() == "sensor_rtt");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Stream one sensor metrics over RTT");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called without arguments") {
      SECTION("Should display usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt")};
        command.Run(1, argv, stream);
        REQUIRE(stream.output().find("usage:") != std::string::npos);
      }
    }

    SECTION("When called with 'off'") {
      SECTION("Should request off and return ok") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("off")};
        command.Run(2, argv, stream);
        REQUIRE(control.off_requested_);
        REQUIRE(stream.output() == "ok\r\n");
      }
    }

    SECTION("When called with 'status'") {
      SECTION("Should display status 'off' when disabled") {
        control.status_.enabled = false;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("status")};
        command.Run(2, argv, stream);
        REQUIRE(stream.output() == "off\r\n");
      }

      SECTION("Should display status 'on' when enabled") {
        control.status_.enabled = true;
        control.status_.sensor_id = 1;
        control.status_.output_hz = 2000;
        control.status_.dropped_frames = 3;
        control.status_.backlog_frames = 7;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("status")};
        command.Run(2, argv, stream);
        REQUIRE(stream.output() == "on id=1 output_hz=2000 dropped=3 backlog=7\r\n");
      }
    }

    SECTION("When called with a valid sensor id") {
      SECTION("Should request observe for that id") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2")};
        command.Run(2, argv, stream);
        REQUIRE(control.observe_requested_);
        REQUIRE(control.last_observe_id_ == 2);
        REQUIRE(stream.output() == "ok\r\n");
      }

      SECTION("With an extra argument, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2"),
                        const_cast<char*>("adc")};
        command.Run(3, argv, stream);
        REQUIRE_FALSE(control.observe_requested_);
        REQUIRE(stream.output().find("usage:") != std::string::npos);
      }
    }

    SECTION("When called with an unknown sensor id") {
      SECTION("Should return an error") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("3")};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(control.observe_requested_);
        REQUIRE(stream.output() == "error: unknown sensor id\r\n");
      }
    }

    SECTION("When called with 'freq'") {
      SECTION("Without value, should return current frequency in Hz") {
        control.status_.output_hz = 100;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq")};
        command.Run(2, argv, stream);
        REQUIRE(stream.output() == "100\r\n");
      }

      SECTION("With value, should set frequency") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("200")};
        command.Run(3, argv, stream);
        REQUIRE(control.output_hz_requested_);
        REQUIRE(control.last_output_hz_ == 200);
        REQUIRE(stream.output() == "ok\r\n");
      }

      SECTION("With 'max', should set frequency to ADC channel rate") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("max")};
        command.Run(3, argv, stream);
        REQUIRE(control.output_hz_requested_);
        REQUIRE(control.last_output_hz_ ==
                ::midismith::adc_board::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
        REQUIRE(stream.output() == "ok\r\n");
      }

      SECTION("With invalid value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("abc")};
        command.Run(3, argv, stream);
        REQUIRE_FALSE(control.output_hz_requested_);
        REQUIRE(stream.output().find("usage:") != std::string::npos);
      }

      SECTION("With zero value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("0")};
        command.Run(3, argv, stream);
        REQUIRE_FALSE(control.output_hz_requested_);
        REQUIRE(stream.output().find("usage:") != std::string::npos);
      }
    }
  }
}
#endif
