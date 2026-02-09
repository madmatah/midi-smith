#include "app/shell/commands/sensor_rtt_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

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
  bool RequestObserve(std::uint8_t sensor_id,
                      domain::sensors::SensorRttMode mode) noexcept override {
    last_observe_id = sensor_id;
    last_mode = mode;
    observe_requested = true;
    return true;
  }
  bool RequestSetPeriod(std::uint32_t period_ms) noexcept override {
    last_period_ms = period_ms;
    period_requested = true;
    return true;
  }
  app::telemetry::SensorRttTelemetryStatus GetStatus() const noexcept override {
    return status;
  }

  app::telemetry::SensorRttTelemetryStatus status{};
  bool off_requested = false;
  bool observe_requested = false;
  bool period_requested = false;
  std::uint8_t last_observe_id = 0;
  domain::sensors::SensorRttMode last_mode = domain::sensors::SensorRttMode::kRaw;
  std::uint32_t last_period_ms = 0;
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
        control.status.mode = domain::sensors::SensorRttMode::kProcessed;
        control.status.period_ms = 10;
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("status")};
        cmd.Run(2, argv, stream);
        REQUIRE(stream.GetOutput() == "on id=1 mode=processed period_ms=10\r\n");
      }
    }

    SECTION("When called with a valid sensor id") {
      SECTION("Should request observe for that id with default mode 'processed'") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2")};
        cmd.Run(2, argv, stream);
        REQUIRE(control.observe_requested);
        REQUIRE(control.last_observe_id == 2);
        REQUIRE(control.last_mode == domain::sensors::SensorRttMode::kProcessed);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("Should request observe for that id with mode 'raw'") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2"),
                        const_cast<char*>("raw")};
        cmd.Run(3, argv, stream);
        REQUIRE(control.observe_requested);
        REQUIRE(control.last_observe_id == 2);
        REQUIRE(control.last_mode == domain::sensors::SensorRttMode::kRaw);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("Should request observe for that id with mode 'processed'") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2"),
                        const_cast<char*>("processed")};
        cmd.Run(3, argv, stream);
        REQUIRE(control.observe_requested);
        REQUIRE(control.last_observe_id == 2);
        REQUIRE(control.last_mode == domain::sensors::SensorRttMode::kProcessed);
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("With an invalid mode, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("2"),
                        const_cast<char*>("invalid")};
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
        control.status.period_ms = 10;  // 100Hz
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq")};
        cmd.Run(2, argv, stream);
        REQUIRE(stream.GetOutput() == "100\r\n");
      }

      SECTION("With value, should set frequency") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("200")};
        cmd.Run(3, argv, stream);
        REQUIRE(control.period_requested);
        REQUIRE(control.last_period_ms == 5);  // 1000/200 = 5ms
        REQUIRE(stream.GetOutput() == "ok\r\n");
      }

      SECTION("With invalid value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("abc")};
        cmd.Run(3, argv, stream);
        REQUIRE_FALSE(control.period_requested);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }

      SECTION("With zero value, should show usage") {
        char* argv[] = {const_cast<char*>("sensor_rtt"), const_cast<char*>("freq"),
                        const_cast<char*>("0")};
        cmd.Run(3, argv, stream);
        REQUIRE_FALSE(control.period_requested);
        REQUIRE(stream.GetOutput().find("usage:") != std::string::npos);
      }
    }
  }
}
