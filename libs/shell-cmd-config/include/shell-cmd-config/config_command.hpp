#pragma once

#include "config/transactional_config_dictionary.hpp"
#include "shell/command_requirements.hpp"

namespace midismith::shell_cmd_config {

class ConfigCommand final : public midismith::shell::CommandRequirements {
 public:
  explicit ConfigCommand(midismith::config::TransactionalConfigDictionary& provider) noexcept
      : provider_(provider) {}

  std::string_view Name() const noexcept override {
    return "config";
  }

  std::string_view Help() const noexcept override {
    return "Manage persistent configuration (getall/get/set/save)";
  }

  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  midismith::config::TransactionalConfigDictionary& provider_;
};

}  // namespace midismith::shell_cmd_config
