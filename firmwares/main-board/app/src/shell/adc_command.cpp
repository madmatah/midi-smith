#include "app/shell/adc_command.hpp"

#include <cstdio>
#include <optional>
#include <string_view>

#include "app/adc/adc_board_state.hpp"

namespace midismith::main_board::app::shell {

namespace {

constexpr std::string_view kUsage =
    "usage: adc <autostart|stop|poweron|poweroff|status> [id]\r\n"
    "  autostart    Start sequential power-on sequence for all ADC boards\r\n"
    "  stop         Power off all ADC boards and reset the sequence\r\n"
    "  poweron <id> Power on a specific ADC board (id: 1-8)\r\n"
    "  poweroff <id> Power off a specific ADC board (id: 1-8)\r\n"
    "  status       Show power state of all ADC boards\r\n";

std::string_view StateLabel(midismith::main_board::app::adc::AdcBoardState state) noexcept {
  switch (state) {
    case midismith::main_board::app::adc::AdcBoardState::kElectricallyOff:
      return "off";
    case midismith::main_board::app::adc::AdcBoardState::kElectricallyOn:
      return "on";
    case midismith::main_board::app::adc::AdcBoardState::kUnresponsive:
      return "unresponsive";
    case midismith::main_board::app::adc::AdcBoardState::kInitializing:
      return "initializing";
    case midismith::main_board::app::adc::AdcBoardState::kReady:
      return "ready";
    case midismith::main_board::app::adc::AdcBoardState::kAcquiring:
      return "acquiring";
  }
  return "unknown";
}

std::optional<std::uint8_t> ParseBoardId(int argc, char** argv) noexcept {
  if (argc < 3) {
    return std::nullopt;
  }
  const std::string_view arg(argv[2]);
  if (arg.size() == 1 && arg[0] >= '1' && arg[0] <= '8') {
    return static_cast<std::uint8_t>(arg[0] - '0');
  }
  return std::nullopt;
}

}  // namespace

AdcCommand::AdcCommand(AdcBoardsControlRequirements& boards_control) noexcept
    : boards_control_(boards_control) {}

void AdcCommand::Run(int argc, char** argv,
                     midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2) {
    out.Write(kUsage);
    return;
  }

  const std::string_view subcommand(argv[1]);

  if (subcommand == "autostart") {
    StartSequence(out);
  } else if (subcommand == "stop") {
    StopAllBoards(out);
  } else if (subcommand == "poweron") {
    PowerOnBoard(argc, argv, out);
  } else if (subcommand == "poweroff") {
    PowerOffBoard(argc, argv, out);
  } else if (subcommand == "status") {
    PrintBoardStatusTable(out);
  } else {
    out.Write(kUsage);
  }
}

void AdcCommand::StartSequence(midismith::io::WritableStreamRequirements& out) noexcept {
  boards_control_.StartPowerSequence();
  out.Write("Power sequence started\r\n");
}

void AdcCommand::StopAllBoards(midismith::io::WritableStreamRequirements& out) noexcept {
  boards_control_.StopAll();
  out.Write("All boards powered off\r\n");
}

void AdcCommand::PowerOnBoard(int argc, char** argv,
                              midismith::io::WritableStreamRequirements& out) noexcept {
  const auto board_id = ParseBoardId(argc, argv);
  if (!board_id.has_value()) {
    out.Write(kUsage);
    return;
  }
  boards_control_.PowerOn(*board_id);
}

void AdcCommand::PowerOffBoard(int argc, char** argv,
                               midismith::io::WritableStreamRequirements& out) noexcept {
  const auto board_id = ParseBoardId(argc, argv);
  if (!board_id.has_value()) {
    out.Write(kUsage);
    return;
  }
  boards_control_.PowerOff(*board_id);
}

void AdcCommand::PrintBoardStatusTable(midismith::io::WritableStreamRequirements& out) noexcept {
  for (std::uint8_t id = 1; id <= 8; ++id) {
    const auto state = boards_control_.board_state(id);
    char line[32];
    const auto written =
        std::snprintf(line, sizeof(line), "  board %u: %s\r\n", id, StateLabel(state).data());
    if (written > 0) {
      out.Write(std::string_view(line, static_cast<std::size_t>(written)));
    }
  }
}

}  // namespace midismith::main_board::app::shell
