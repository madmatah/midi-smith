#include "app/shell/ui_command.hpp"

#include <charconv>
#include <cstring>
#include <optional>

#include "io/stream_requirements.hpp"

namespace midismith::main_board::app::shell {

namespace {

constexpr std::string_view kUsage = "usage: ui rotate <detents> | ui press | ui hold\r\n";

void PrintUsage(midismith::io::WritableStreamRequirements& out) noexcept {
  out.Write(kUsage.data());
}

std::optional<std::int16_t> ParseDetents(const char* text) noexcept {
  int parsed_value = 0;
  const char* text_end = text + std::strlen(text);
  const auto result = std::from_chars(text, text_end, parsed_value);
  if (result.ec != std::errc{} || result.ptr != text_end || parsed_value == 0 ||
      parsed_value < INT16_MIN || parsed_value > INT16_MAX) {
    return std::nullopt;
  }
  return static_cast<std::int16_t>(parsed_value);
}

}  // namespace

UiCommand::UiCommand(
    midismith::os::QueueRequirements<midismith::menu::InputEvent>& input_queue) noexcept
    : input_queue_(input_queue) {}

std::string_view UiCommand::Name() const noexcept {
  return "ui";
}

std::string_view UiCommand::Help() const noexcept {
  return "Drive the local UI (rotate <detents>, press, hold)";
}

void UiCommand::Run(int argc, char** argv,
                    midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2) {
    PrintUsage(out);
    return;
  }
  const std::string_view subcommand(argv[1]);
  midismith::menu::InputEvent event = midismith::menu::InputEvent::ButtonPress();
  if (subcommand == "rotate") {
    if (argc < 3) {
      PrintUsage(out);
      return;
    }
    const auto detents = ParseDetents(argv[2]);
    if (!detents.has_value()) {
      PrintUsage(out);
      return;
    }
    event = midismith::menu::InputEvent::Rotate(*detents);
  } else if (subcommand == "press") {
    event = midismith::menu::InputEvent::ButtonPress();
  } else if (subcommand == "hold") {
    event = midismith::menu::InputEvent::ButtonLongPress();
  } else {
    PrintUsage(out);
    return;
  }
  if (!input_queue_.Send(event, 0)) {
    out.Write("ui: input queue full\r\n");
    return;
  }
  out.Write("ok\r\n");
}

}  // namespace midismith::main_board::app::shell
