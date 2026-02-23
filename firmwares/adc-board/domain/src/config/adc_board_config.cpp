#include "domain/config/adc_board_config.hpp"

#include <cstring>

#include "config/config_validator.hpp"

namespace midismith::adc_board::domain::config {

bool IsValidBoardId(std::uint8_t board_id) noexcept {
  return board_id >= kMinBoardId && board_id <= kMaxBoardId;
}

AdcBoardConfig CreateDefaultAdcBoardConfig() noexcept {
  AdcBoardConfig config{};
  std::memset(&config, 0, sizeof(config));

  config.magic_number = AdcBoardConfig::kMagicNumber;
  config.version = AdcBoardConfig::kVersion;
  config.data.can_board_id = kDefaultBoardId;

  midismith::config::ConfigValidator<AdcBoardConfig>::StampCrc(config);
  return config;
}

AdcBoardConfig MigrateAdcBoardConfig(const AdcBoardConfig& old_config,
                                     std::uint16_t old_version) noexcept {
  AdcBoardConfig migrated = CreateDefaultAdcBoardConfig();

  if (old_version >= 1) {
    if (IsValidBoardId(old_config.data.can_board_id)) {
      migrated.data.can_board_id = old_config.data.can_board_id;
    }
  }

  midismith::config::ConfigValidator<AdcBoardConfig>::StampCrc(migrated);
  return migrated;
}

}  // namespace midismith::adc_board::domain::config
