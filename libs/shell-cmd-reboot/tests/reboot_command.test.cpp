#if defined(UNIT_TESTS)

#include "shell-cmd-reboot/reboot_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

#include "bsp-types/board_reset_requirements.hpp"
#include "io/stream_requirements.hpp"

namespace {

class RecordingBoardReset final : public midismith::bsp::BoardResetRequirements {
 public:
  void ResetBoard() noexcept override {
    reset_called_ = true;
  }

  bool reset_called() const noexcept {
    return reset_called_;
  }

 private:
  bool reset_called_ = false;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char c) noexcept override {
    output_ += c;
  }

  void Write(const char* str) noexcept override {
    if (str != nullptr) {
      output_ += str;
    }
  }

  const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

}  // namespace

TEST_CASE("The RebootCommand class", "[libs][shell-cmd-reboot]") {
  RecordingBoardReset board_reset;
  RecordingStream stream;
  midismith::shell_cmd_reboot::RebootCommand command(board_reset);

  SECTION("The Name() method") {
    SECTION("When called") {
      SECTION("Should return reboot") {
        REQUIRE(command.Name() == "reboot");
      }
    }
  }

  SECTION("The Help() method") {
    SECTION("When called") {
      SECTION("Should return the expected help string") {
        REQUIRE(command.Help() == "Reboot board (software reset)");
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When called with no extra argument") {
      SECTION("Should print rebooting and call board reset") {
        char arg0[] = "reboot";
        char* argv[] = {arg0};

        command.Run(1, argv, stream);

        REQUIRE(stream.output() == "rebooting...\r\n");
        REQUIRE(board_reset.reset_called());
      }
    }

    SECTION("When called with extra arguments") {
      SECTION("Should print usage and not call board reset") {
        char arg0[] = "reboot";
        char arg1[] = "now";
        char* argv[] = {arg0, arg1};

        command.Run(2, argv, stream);

        REQUIRE(stream.output() == "usage: reboot\r\n");
        REQUIRE_FALSE(board_reset.reset_called());
      }
    }
  }
}

#endif
