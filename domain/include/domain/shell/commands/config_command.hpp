#pragma once

#include "domain/config/transactional_config_dictionary.hpp"
#include "shell/command_requirements.hpp"

namespace domain::shell::commands {

class ConfigCommand final : public ::shell::CommandRequirements {
 public:
  explicit ConfigCommand(domain::config::TransactionalConfigDictionary& provider) noexcept
      : provider_(provider) {}

  std::string_view Name() const noexcept override {
    return "config";
  }

  std::string_view Help() const noexcept override {
    return "Manage persistent configuration (getall/get/set/save)";
  }

  void Run(int argc, char** argv, domain::io::WritableStreamRequirements& out) noexcept override;

 private:
  domain::config::TransactionalConfigDictionary& provider_;
};

}  // namespace domain::shell::commands
