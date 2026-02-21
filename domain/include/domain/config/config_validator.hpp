#ifndef DOMAIN_CONFIG_CONFIG_VALIDATOR_HPP
#define DOMAIN_CONFIG_CONFIG_VALIDATOR_HPP

#include <cstdint>

#include "domain/config/user_config.hpp"

namespace domain::config {

enum class ConfigStatus {
  kValid,
  kVirginFlash,
  kInvalidMagic,
  kInvalidCrc,
  kOlderVersion,
};

ConfigStatus ValidateConfig(const UserConfig& config) noexcept;
bool IsValidBoardId(std::uint8_t board_id) noexcept;
UserConfig MigrateConfig(const UserConfig& old_config, std::uint16_t old_version) noexcept;

}  // namespace domain::config

#endif  // DOMAIN_CONFIG_CONFIG_VALIDATOR_HPP
