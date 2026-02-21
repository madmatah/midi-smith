#include "domain/shell/commands/version_command.hpp"

namespace domain::shell::commands {

VersionCommand::VersionCommand(std::string_view full_version, std::string_view build_type,
                               std::string_view commit_date) noexcept
    : _full_version(full_version), _build_type(build_type), _commit_date(commit_date) {}

void VersionCommand::Run(int, char**, domain::io::WritableStreamRequirements& out) noexcept {
  out.Write("Firmware Version: ");
  out.Write(_full_version);
  out.Write("\r\nBuild Type: ");
  out.Write(_build_type);
  out.Write("\r\nCommit Date: ");
  out.Write(_commit_date);
  out.Write("\r\n");
}

}  // namespace domain::shell::commands
