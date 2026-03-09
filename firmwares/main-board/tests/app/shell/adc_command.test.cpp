#if defined(UNIT_TESTS)

#include "app/shell/adc_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>

#include "app/shell/adc_boards_control_requirements.hpp"
#include "domain/adc/adc_board_state.hpp"
#include "io/stream_requirements.hpp"

namespace {

using AdcBoardState = midismith::main_board::domain::adc::AdcBoardState;

class RecordingBoardsControl final
    : public midismith::main_board::app::shell::AdcBoardsControlRequirements {
 public:
  void StartPowerSequence() noexcept override {
    start_power_sequence_called_ = true;
  }

  void StopAll() noexcept override {
    stop_all_called_ = true;
  }

  void PowerOn(std::uint8_t peer_id) noexcept override {
    power_on_peer_id_ = peer_id;
  }

  void PowerOff(std::uint8_t peer_id) noexcept override {
    power_off_peer_id_ = peer_id;
  }

  [[nodiscard]] AdcBoardState board_state(std::uint8_t) const noexcept override {
    return AdcBoardState::kElectricallyOff;
  }

  [[nodiscard]] bool start_power_sequence_called() const noexcept {
    return start_power_sequence_called_;
  }

  [[nodiscard]] bool stop_all_called() const noexcept {
    return stop_all_called_;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& power_on_peer_id() const noexcept {
    return power_on_peer_id_;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& power_off_peer_id() const noexcept {
    return power_off_peer_id_;
  }

 private:
  bool start_power_sequence_called_ = false;
  bool stop_all_called_ = false;
  std::optional<std::uint8_t> power_on_peer_id_;
  std::optional<std::uint8_t> power_off_peer_id_;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    output_ += str;
  }

  [[nodiscard]] bool has_output() const noexcept {
    return !output_.empty();
  }

  [[nodiscard]] const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

}  // namespace

using midismith::main_board::app::shell::AdcCommand;

TEST_CASE("AdcCommand — Name()") {
  RecordingBoardsControl control;
  AdcCommand command(control);

  SECTION("Should return \"adc\"") {
    REQUIRE(command.Name() == "adc");
  }
}

TEST_CASE("AdcCommand — Run()") {
  RecordingBoardsControl control;
  RecordingStream stream;
  AdcCommand command(control);

  SECTION("When no subcommand is provided") {
    SECTION("Should print usage") {
      char argv0[] = "adc";
      char* argv[] = {argv0};
      command.Run(1, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("When the subcommand is autostart") {
    SECTION("Should call StartPowerSequence") {
      char argv0[] = "adc";
      char argv1[] = "autostart";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(control.start_power_sequence_called());
    }
  }

  SECTION("When the subcommand is stop") {
    SECTION("Should call StopAll") {
      char argv0[] = "adc";
      char argv1[] = "stop";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(control.stop_all_called());
    }
  }

  SECTION("When the subcommand is poweron") {
    SECTION("With a valid board id") {
      SECTION("Should call PowerOn with that id") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char argv2[] = "3";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(control.power_on_peer_id().has_value());
        REQUIRE(*control.power_on_peer_id() == 3);
      }
    }

    SECTION("With id 1 (minimum valid)") {
      SECTION("Should call PowerOn with 1") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char argv2[] = "1";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(control.power_on_peer_id().has_value());
        REQUIRE(*control.power_on_peer_id() == 1);
      }
    }

    SECTION("With id 8 (maximum valid)") {
      SECTION("Should call PowerOn with 8") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char argv2[] = "8";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(control.power_on_peer_id().has_value());
        REQUIRE(*control.power_on_peer_id() == 8);
      }
    }

    SECTION("With no id argument") {
      SECTION("Should print usage and not call PowerOn") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(control.power_on_peer_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("With invalid id '0'") {
      SECTION("Should print usage and not call PowerOn") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char argv2[] = "0";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE_FALSE(control.power_on_peer_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("With invalid id '9'") {
      SECTION("Should print usage and not call PowerOn") {
        char argv0[] = "adc";
        char argv1[] = "poweron";
        char argv2[] = "9";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE_FALSE(control.power_on_peer_id().has_value());
        REQUIRE(stream.has_output());
      }
    }
  }

  SECTION("When the subcommand is poweroff") {
    SECTION("With a valid board id") {
      SECTION("Should call PowerOff with that id") {
        char argv0[] = "adc";
        char argv1[] = "poweroff";
        char argv2[] = "5";
        char* argv[] = {argv0, argv1, argv2};
        command.Run(3, argv, stream);
        REQUIRE(control.power_off_peer_id().has_value());
        REQUIRE(*control.power_off_peer_id() == 5);
      }
    }

    SECTION("With no id argument") {
      SECTION("Should print usage and not call PowerOff") {
        char argv0[] = "adc";
        char argv1[] = "poweroff";
        char* argv[] = {argv0, argv1};
        command.Run(2, argv, stream);
        REQUIRE_FALSE(control.power_off_peer_id().has_value());
        REQUIRE(stream.has_output());
      }
    }
  }

  SECTION("When the subcommand is status") {
    SECTION("Should produce output for all 8 boards") {
      char argv0[] = "adc";
      char argv1[] = "status";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.has_output());
    }
  }

  SECTION("When the subcommand is unknown") {
    SECTION("Should print usage") {
      char argv0[] = "adc";
      char argv1[] = "invalid";
      char* argv[] = {argv0, argv1};
      command.Run(2, argv, stream);
      REQUIRE(stream.has_output());
      REQUIRE_FALSE(control.start_power_sequence_called());
      REQUIRE_FALSE(control.power_on_peer_id().has_value());
      REQUIRE_FALSE(control.power_off_peer_id().has_value());
    }
  }
}

#endif
