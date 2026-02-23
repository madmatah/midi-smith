#pragma once

#include <cstddef>
#include <string_view>

#include "io/stream_requirements.hpp"
#include "shell/command_requirements.hpp"
#include "shell/commands/help_command.hpp"
#include "shell/help_provider.hpp"

namespace midismith::shell {

template <std::size_t kMaxCommands>
class CommandDispatcher : public HelpProvider {
 public:
  CommandDispatcher() noexcept : help_command_(*this), count_(0) {
    Register(help_command_);
  }

  bool Register(CommandRequirements& command) noexcept {
    if (count_ >= kMaxCommands) {
      return false;
    }

    std::size_t i = count_;
    while (i > 0 && command.Name() < commands_[i - 1]->Name()) {
      commands_[i] = commands_[i - 1];
      i--;
    }
    commands_[i] = &command;
    count_++;

    return true;
  }

  void Dispatch(int argc, char** argv,
                midismith::io::WritableStreamRequirements& out) const noexcept {
    if (argc <= 0) {
      return;
    }

    std::string_view cmd_name(argv[0]);

    for (std::size_t i = 0; i < count_; ++i) {
      if (cmd_name == commands_[i]->Name()) {
        commands_[i]->Run(argc, argv, out);
        return;
      }
    }

    out.Write("error: command not found: ");
    out.Write(argv[0]);
    out.Write("\r\n");
  }

  std::size_t FindCompletions(std::string_view prefix, CommandRequirements** matches,
                              std::size_t max_matches) const noexcept {
    if (prefix.empty()) {
      return 0;
    }

    std::size_t found = 0;

    for (std::size_t i = 0; i < count_; ++i) {
      std::string_view name = commands_[i]->Name();
      if (name.compare(0, prefix.length(), prefix) == 0) {
        if (found < max_matches) {
          matches[found++] = commands_[i];
        }
      }
    }

    return found;
  }

  void ShowHelp(midismith::io::WritableStreamRequirements& out) const noexcept override {
    out.Write("Available commands:\r\n");
    for (std::size_t i = 0; i < count_; ++i) {
      out.Write("  ");
      std::string_view name = commands_[i]->Name();
      out.Write(name);

      WritePadding(out, name.length());

      out.Write(commands_[i]->Help());
      out.Write("\r\n");
    }
  }

 private:
  void WritePadding(midismith::io::WritableStreamRequirements& out,
                    std::size_t name_length) const noexcept {
    const std::size_t kNameColumnWidth = 20;
    for (std::size_t j = name_length; j < kNameColumnWidth; ++j) {
      out.Write(' ');
    }
  }

  commands::HelpCommand help_command_;
  CommandRequirements* commands_[kMaxCommands];
  std::size_t count_;
};

}  // namespace midismith::shell
