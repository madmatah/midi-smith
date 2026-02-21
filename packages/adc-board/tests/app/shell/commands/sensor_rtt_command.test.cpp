#include "app/shell/commands/sensor_rtt_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "app/config/analog_acquisition.hpp"
#include "app/telemetry/sensor_rtt_telemetry_control_requirements.hpp"
#include "domain/io/stream_requirements.hpp"
#include "domain/sensors/sensor_registry.hpp"
#include "domain/sensors/sensor_state.hpp"

namespace {

class StreamStub : public domain::io::StreamRequirements {
 public:
  domain::io::ReadResult Read(std::uint8_t&) noexcept override {
    return domain::io::ReadResult::kNoData;
  }
  void Write(char c) noexcept override {
    output_ += c;
  }
  void Write(const char* str) noexcept override {
    output_ += str;
  }
  void Clear() {
    output_.clear();
  }
  const std::string& GetOutput() const {
    return output_;
  }

 private:
  std::string output_;
};

class ControlMock : public app::telemetry::SensorRttTelemetryControlRequirements {
 public:
  bool RequestOff() noexcept override {
    off_requested = true;
    return true;
  }
  bool RequestObserve(std::uint8_t sensor_id) noexcept override {
    last_observe_id = sensor_id;
    observe_requested = true;
    return true;
  }
  bool RequestSetOutputHz(std::uint32_t output_hz) noexcept override {
    last_output_hz = output_hz;
    output_hz_requested = true;
    return true;
  }
  app::telemetry::SensorRttTelemetryStatus GetStatus() const noexcept override {
    return status;
  }

  app::telemetry::SensorRttTelemetryStatus status{};
  bool off_requested = false;
  bool observe_requested = false;
  bool output_hz_requested = false;
  std::uint8_t last_observe_id = 0;
  std::uint32_t last_output_hz = 0;
};

}  // namespace

TEST_CASE("The SensorRttCommand class", "[app][shell][commands]") {
  domain::sensors::SensorState sensors[] = {domain::sensors::SensorState{.id = 1},
                                            domain::sensors::SensorState{.id = 2}};
  domain::sensors::SensorRegistry registry(sensors, 2);
  ControlMock control;
  app::shell::commands::SensorRttCommand cmd(registry, control);
  StreamStub stream;

  SECTION("The Name() method") {
    SECTION("Should return 'sensor_rtt'") {
      REQUIRE(cmd.Name() == "sensor_rtt");
    }
  }

  SECTION("The Run() method") {
    SECTION("When called without arguments") {
      SECTION("Should display usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt")};
        cmd.Run(1, argv, stream);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }
    }

    SECTION("When called with 'off'") {
      SECTION("Should request off and return ok") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("off")};
        cmd.Run(2, argv, stream);
        REQUIRE(control.off_requested);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }
    }

    SECTION("When called with 'status'") {
      SECTION("Should display status 'off' when disabled") {
        control.status.enabled = false;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("status")};
        cmd.Run(2, argv, stream);
        REQUIRE(stream.GetOutput() == "off\r\n");
      }

      SECTION("Should display status 'on' when enabled") {
        control.status.enabled = true;
        control.status.sensor_id = 1;
        control.status.output_hz = 2000;
        control.status.dropped_frames = 3;
        control.status.backlog_frames = 7;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("status")};
        cmd.Run(2, argv, stream);
        REQUIRE(stream.GetOutput() == "on id=1 output_hz=2000 dropped=3 backlog=7\r\n");
      }
    }

    SECTION("When called with a valid sensor id") {
      SECTION("Should request observe for that id") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2")};
        cmd.Run(2, argv, stream);
        REQUIRE(control.observe_requested);
        REQUIRE(control.last_observe_id == 2);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("With an extra argument, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2"),
                        const_cast<char*>("adc")};
        cmd.Run(3, argv, stream);
        REQUIRE_FALSE(control.observe_requested);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }
    }

    SECTION("When called with an unknown sensor id") {
      SECTION("Should return an error") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("3")};
        cmd.Run(2, argv, stream);
        REQUIRE_FALSE(control.observe_requested);
        REQUIRE(stream.GetOutput() == "error: unknown sensor id\r\n");
      }
    }

    SECTION("When called with 'freq'") {
      SECTION("Without value, should return current frequency in Hz") {
        control.status.output_hz = 100;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq")};
        cmd.Run(2, argv, stream);
        REQUIRE(stream.GetOutput() == "100\r\n");
      }

      SECTION("With value, should set frequency") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("200")};
        cmd.Run(3, argv, stream);
        REQUIRE(control.output_hz_requested);
        REQUIRE(control.last_output_hz == 200);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("With 'max', should set frequency to ADC channel rate") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("max")};
        cmd.Run(3, argv, stream);
        REQUIRE(control.output_hz_requested);
        REQUIRE(control.last_output_hz == ::app::config::ANALOG_ACQUISITION_CHANNEL_RATE_HZ);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("With invalid value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("abc")};
        cmd.Run(3, argv, stream);
        REQUIRE_FALSE(control.output_hz_requested);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }

      SECTION("With zero value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("0")};
        cmd.Run(3, argv, stream);
        REQUIRE_FALSE(control.output_hz_requested);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }
    }
  }
}
