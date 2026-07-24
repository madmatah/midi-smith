#if defined(UNIT_TESTS)

#include "app/shell/ui_command.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <string>
#include <vector>

#include "io/stream_requirements.hpp"

namespace {

using Catch::Matchers::ContainsSubstring;
using midismith::menu::InputEvent;

class RecordingInputQueue final : public midismith::os::QueueRequirements<InputEvent> {
 public:
  bool Send(const InputEvent& item, std::uint32_t timeout_ms) noexcept override {
    static_cast<void>(timeout_ms);
    if (!accept_sends) {
      return false;
    }
    sent_events.push_back(item);
    return true;
  }

  bool SendFromIsr(const InputEvent& item) noexcept override {
    return Send(item, 0);
  }

  bool Receive(InputEvent& item, std::uint32_t timeout_ms) noexcept override {
    static_cast<void>(item);
    static_cast<void>(timeout_ms);
    return false;
  }

  std::vector<InputEvent> sent_events;
  bool accept_sends = true;
};

class RecordingStream final : public midismith::io::WritableStreamRequirements {
 public:
  void Write(char character) noexcept override {
    output_ += character;
  }

  void Write(const char* text) noexcept override {
    output_ += text;
  }

  [[nodiscard]] const std::string& output() const noexcept {
    return output_;
  }

 private:
  std::string output_;
};

}  // namespace

TEST_CASE("The UiCommand class") {
  SECTION("The Run() method") {
    SECTION("When the subcommand is rotate with a detent count") {
      SECTION("Should queue a rotate event with the parsed detents") {
        RecordingInputQueue queue;
        RecordingStream stream;
        midismith::main_board::app::shell::UiCommand command(queue);
        char argv0[] = "ui";
        char argv1[] = "rotate";
        char argv2[] = "-3";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(queue.sent_events.size() == 1);
        REQUIRE(queue.sent_events[0].kind == InputEvent::Kind::kRotate);
        REQUIRE(queue.sent_events[0].detents == -3);
        REQUIRE_THAT(stream.output(), ContainsSubstring("ok"));
      }
    }

    SECTION("When the subcommand is press") {
      SECTION("Should queue a button press event") {
        RecordingInputQueue queue;
        RecordingStream stream;
        midismith::main_board::app::shell::UiCommand command(queue);
        char argv0[] = "ui";
        char argv1[] = "press";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(queue.sent_events.size() == 1);
        REQUIRE(queue.sent_events[0].kind == InputEvent::Kind::kButtonPress);
      }
    }

    SECTION("When the subcommand is hold") {
      SECTION("Should queue a long press event") {
        RecordingInputQueue queue;
        RecordingStream stream;
        midismith::main_board::app::shell::UiCommand command(queue);
        char argv0[] = "ui";
        char argv1[] = "hold";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE(queue.sent_events.size() == 1);
        REQUIRE(queue.sent_events[0].kind == InputEvent::Kind::kButtonLongPress);
      }
    }

    SECTION("When the arguments are invalid") {
      SECTION("Should print the usage and queue nothing") {
        RecordingInputQueue queue;
        RecordingStream stream;
        midismith::main_board::app::shell::UiCommand command(queue);
        char argv0[] = "ui";
        char argv1[] = "rotate";
        char argv2[] = "abc";
        char* argv[] = {argv0, argv1, argv2};

        command.Run(3, argv, stream);

        REQUIRE(queue.sent_events.empty());
        REQUIRE_THAT(stream.output(), ContainsSubstring("usage"));
      }
    }

    SECTION("When the input queue is full") {
      SECTION("Should report the failure") {
        RecordingInputQueue queue;
        queue.accept_sends = false;
        RecordingStream stream;
        midismith::main_board::app::shell::UiCommand command(queue);
        char argv0[] = "ui";
        char argv1[] = "press";
        char* argv[] = {argv0, argv1};

        command.Run(2, argv, stream);

        REQUIRE_THAT(stream.output(), ContainsSubstring("full"));
      }
    }
  }
}

#endif
