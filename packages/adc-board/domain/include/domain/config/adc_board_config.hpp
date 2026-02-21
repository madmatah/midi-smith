#pragma once

#include <cstdint>

#include "domain/config/storable_config.hpp"

namespace domain::config {

inline constexpr std::uint8_t kMinBoardId = 1;
inline constexpr std::uint8_t kMaxBoardId = 8;
inline constexpr std::uint8_t kDefaultBoardId = 1;

struct AdcBoardData {
  std::uint8_t can_board_id;
};

inline constexpr std::uint32_t kAdcMagic = 0x41444321u;
inline constexpr std::uint16_t kAdcVersion = 1;

using AdcBoardConfig = StorableConfig<AdcBoardData, kAdcMagic, kAdcVersion>;
static_assert(kStorableConfigLayoutValid<AdcBoardData, kAdcMagic, kAdcVersion>);

bool IsValidBoardId(std::uint8_t board_id) noexcept;
AdcBoardConfig CreateDefaultAdcBoardConfig() noexcept;
AdcBoardConfig MigrateAdcBoardConfig(const AdcBoardConfig& old_config,
                                     std::uint16_t old_version) noexcept;

}  // namespace domain::config
