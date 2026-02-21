#include "domain/config/config_validator.hpp"

#include <cstring>

#include "domain/hash/crc32.hpp"

namespace domain::config {

ConfigStatus ValidateConfig(const UserConfig& config) noexcept {
  constexpr std::uint32_t kErasedFlashWord = 0xFFFFFFFFu;
  if (config.magic_number == kErasedFlashWord) {
    return ConfigStatus::kVirginFlash;
  }

  if (config.magic_number != kConfigMagicNumber) {
    return ConfigStatus::kInvalidMagic;
  }

  std::uint32_t computed_crc = hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&config),
                                                  offsetof(UserConfig, crc32));
  if (computed_crc != config.crc32) {
    return ConfigStatus::kInvalidCrc;
  }

  if (config.version < kConfigVersion) {
    return ConfigStatus::kOlderVersion;
  }

  return ConfigStatus::kValid;
}

bool IsValidBoardId(std::uint8_t board_id) noexcept {
  return board_id >= kMinBoardId && board_id <= kMaxBoardId;
}

UserConfig MigrateConfig(const UserConfig& old_config, std::uint16_t old_version) noexcept {
  UserConfig migrated = DefaultConfig();

  if (old_version >= 1) {
    if (IsValidBoardId(old_config.data.can_board_id)) {
      migrated.data.can_board_id = old_config.data.can_board_id;
    }
  }

  migrated.crc32 = hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&migrated),
                                      offsetof(UserConfig, crc32));
  return migrated;
}

}  // namespace domain::config
