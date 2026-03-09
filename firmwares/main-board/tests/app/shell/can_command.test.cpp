#if defined(UNIT_TESTS)

#include "app/shell/can_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <string>

#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "io/stream_requirements.hpp"
#include "protocol/messages.hpp"

namespace {

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }

  bool SendStartAdc(std::uint8_t node_id) noexcept override {
    start_adc_node_id_ = node_id;
    return true;
  }

  bool SendStopAdc(std::uint8_t node_id) noexcept override {
    stop_adc_node_id_ = node_id;
    return true;
  }

  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& start_adc_node_id() const noexcept {
    return start_adc_node_id_;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& stop_adc_node_id() const noexcept {
    return stop_adc_node_id_;
  }

 private:
  std::optional<std::uint8_t> start_adc_node_id_;
  std::optional<std::uint8_t> stop_adc_node_id_;
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

 private:
  std::string output_;
};

}  // namespace

using midismith::main_board::app::shell::CanCommand;

TEST_CASE("The CanCommand class") {
  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return 'can'") {
        RecordingMessageSender sender;
        CanCommand command(sender);

        REQUIRE(command.Name() == "can");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        RecordingMessageSender sender;
        CanCommand command(sender);

        REQUIRE(command.Help() == "Send CAN bus commands (can <adc_start|adc_stop> [target])");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When subcommand is adc_start with specific node id") {
      SECTION("Should send start command to that node") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "1";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 1);
      }
    }

    SECTION("When subcommand is adc_start with target all") {
      SECTION("Should send broadcast start command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "all";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 0);
      }
    }

    SECTION("When subcommand is adc_start without target") {
      SECTION("Should send broadcast start command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(sender.start_adc_node_id().has_value());
        REQUIRE(*sender.start_adc_node_id() == 0);
      }
    }

    SECTION("When subcommand is adc_start with invalid target") {
      SECTION("Should print usage and not send command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "bad";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is adc_start with target 0") {
      SECTION("Should print usage and not send command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_start";
        char argv2[] = "0";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is adc_stop with specific node id") {
      SECTION("Should send stop command to that node") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_stop";
        char argv2[] = "3";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(sender.stop_adc_node_id().has_value());
        REQUIRE(*sender.stop_adc_node_id() == 3);
      }
    }

    SECTION("When subcommand is adc_stop without target") {
      SECTION("Should send broadcast stop command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "adc_stop";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(sender.stop_adc_node_id().has_value());
        REQUIRE(*sender.stop_adc_node_id() == 0);
      }
    }

    SECTION("When no subcommand is provided") {
      SECTION("Should print usage and not send command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char* argv[] = {argv0};

        command.Run(1, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE_FALSE(sender.stop_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }

    SECTION("When subcommand is unknown") {
      SECTION("Should print usage and not send command") {
        RecordingMessageSender sender;
        RecordingStream stream;
        CanCommand command(sender);
        char argv0[] = "can";
        char argv1[] = "invalid";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE_FALSE(sender.start_adc_node_id().has_value());
        REQUIRE_FALSE(sender.stop_adc_node_id().has_value());
        REQUIRE(stream.has_output());
      }
    }
  }
}

#endif
