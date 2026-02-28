#include "app/shell/can_command.hpp"

#include <optional>
#include <string_view>

namespace midismith::main_board::app::shell {

namespace {

constexpr std::string_view kUsage =
    "usage: can <adc_start|adc_stop> [target]\r\n"
    "  target: 1-8 or all (default: all)\r\n";

std::optional<std::uint8_t> ParseTarget(int argc, char** argv) noexcept {
  if (argc < 3) return 0;
  const std::string_view arg(argv[2]);
  if (arg == "all") return 0;
  if (arg.size() == 1 && arg[0] >= '1' && arg[0] <= '8') {
    return static_cast<std::uint8_t>(arg[0] - '0');
  }
  return std::nullopt;
}

}  // namespace

CanCommand::CanCommand(messaging::MainBoardMessageSenderRequirements& sender) noexcept
    : sender_(sender) {}

void CanCommand::Run(int argc, char** argv,
                     midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc < 2) {
    out.Write(kUsage);
    return;
  }

  const std::string_view subcommand(argv[1]);
  const auto target_node_id = ParseTarget(argc, argv);

  if (!target_node_id.has_value()) {
    out.Write(kUsage);
    return;
  }

  if (subcommand == "adc_start") {
    sender_.SendStartAdc(*target_node_id);
  } else if (subcommand == "adc_stop") {
    sender_.SendStopAdc(*target_node_id);
  } else {
    out.Write(kUsage);
  }
}

}  // namespace midismith::main_board::app::shell
