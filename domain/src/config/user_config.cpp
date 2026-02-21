#include "domain/config/user_config.hpp"

#include <cstring>

#include "domain/hash/crc32.hpp"

namespace domain::config {

UserConfig DefaultConfig() noexcept {
  UserConfig config{};
  std::memset(&config, 0, sizeof(config));

  config.magic_number = kConfigMagicNumber;
  config.version = kConfigVersion;
  config.data.can_board_id = kDefaultBoardId;

  config.crc32 = hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&config),
                                    offsetof(UserConfig, crc32));
  return config;
}

}  // namespace domain::config
