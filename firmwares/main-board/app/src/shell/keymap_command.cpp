#include "app/shell/keymap_command.hpp"

#include <charconv>
#include <cstdio>
#include <string_view>
#include <system_error>

#include "domain/config/main_board_config.hpp"
#include "io/stream_format.hpp"

namespace midismith::main_board::app::shell {

namespace {

constexpr std::string_view kUsage =
    "usage: keymap <setup|status|cancel>\r\n"
    "  setup <key_count> [start_note]  Begin interactive keymap capture\r\n"
    "    key_count   Number of keys (1-176)\r\n"
    "    start_note  MIDI note for first key (0-127, default 21 (A0))\r\n"
    "  status                          Show keymap state and entries\r\n"
    "  cancel                          Abort in-progress setup\r\n";

constexpr std::uint8_t kDefaultStartNote = 21u;

bool ParseUint8(std::string_view text, std::uint8_t& out_value) noexcept {
  std::uint32_t parsed = 0u;
  const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
  if (result.ec != std::errc() || result.ptr != text.data() + text.size()) {
    return false;
  }
  if (parsed > 255u) {
    return false;
  }
  out_value = static_cast<std::uint8_t>(parsed);
  return true;
}

}  // namespace

KeymapCommand::KeymapCommand(
    midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
    : coordinator_(coordinator),
      stats_provider_(coordinator_),
      providers_{&stats_provider_},
      status_command_("keymap status", "Show keymap state and entries", providers_) {}

void KeymapCommand::Run(int argc, char** argv,
                        midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2) {
    PrintUsage(out);
    return;
  }

  const std::string_view subcommand(argv[1]);

  if (subcommand == "setup") {
    RunSetup(argc, argv, out);
  } else if (subcommand == "status") {
    RunStatus(argc, argv, out);
  } else if (subcommand == "cancel") {
    RunCancel(out);
  } else {
    PrintUsage(out);
  }
}

void KeymapCommand::RunSetup(int argc, char** argv,
                             midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 3) {
    PrintUsage(out);
    return;
  }

  std::uint8_t key_count = 0u;
  if (!ParseUint8(std::string_view(argv[2]), key_count) || key_count < 1u ||
      key_count > midismith::main_board::domain::config::kMaxKeymapEntries) {
    out.Write("error: key_count must be between 1 and 176\r\n");
    return;
  }

  std::uint8_t start_note = kDefaultStartNote;
  if (argc >= 4) {
    if (!ParseUint8(std::string_view(argv[3]), start_note) ||
        start_note > midismith::main_board::domain::config::kMaxMidiNote) {
      out.Write("error: start_note must be between 0 and 127\r\n");
      return;
    }
  }

  if (static_cast<std::uint32_t>(start_note) + key_count > 128u) {
    out.Write("error: start_note + key_count exceeds MIDI note range (max 127)\r\n");
    return;
  }

  if (!coordinator_.StartSetup(key_count, start_note)) {
    out.Write("error: keymap setup already in progress\r\n");
    return;
  }

  out.Write("Please press the ");
  midismith::io::WriteUint8(out, key_count);
  out.Write(" keys, in order, from the first to the last.\r\n");
}

void KeymapCommand::RunStatus(int argc, char** argv,
                              midismith::io::WritableStreamRequirements& out) noexcept {
  // Pass argc=1 so NoArgsRequestParser sees no extra arguments
  status_command_.Run(1, argv, out);
}

void KeymapCommand::RunCancel(midismith::io::WritableStreamRequirements& out) noexcept {
  coordinator_.CancelSetup();
  out.Write("Keymap setup cancelled. Previous configuration restored.\r\n");
}

void KeymapCommand::PrintUsage(midismith::io::WritableStreamRequirements& out) const noexcept {
  out.Write(kUsage);
}

}  // namespace midismith::main_board::app::shell
