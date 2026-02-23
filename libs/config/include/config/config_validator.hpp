#pragma once

#include <cstddef>
#include <cstdint>

#include "config/crc32.hpp"

namespace midismith::config {

enum class ConfigStatus {
  kValid,
  kVirginFlash,
  kInvalidMagic,
  kInvalidCrc,
  kOlderVersion,
  kNewerVersion,
};

template <typename TConfig>
class ConfigValidator {
 public:
  static ConfigStatus Validate(const TConfig& config) noexcept {
    constexpr std::uint32_t kErasedFlashWord = 0xFFFFFFFFu;
    if (config.magic_number == kErasedFlashWord) {
      return ConfigStatus::kVirginFlash;
    }

    if (config.magic_number != TConfig::kMagicNumber) {
      return ConfigStatus::kInvalidMagic;
    }

    std::uint32_t computed_crc = midismith::config::ComputeCrc32(
        reinterpret_cast<const std::uint8_t*>(&config), offsetof(TConfig, crc32));
    if (computed_crc != config.crc32) {
      return ConfigStatus::kInvalidCrc;
    }

    if (config.version < TConfig::kVersion) {
      return ConfigStatus::kOlderVersion;
    }

    if (config.version > TConfig::kVersion) {
      return ConfigStatus::kNewerVersion;
    }

    return ConfigStatus::kValid;
  }

  static void StampCrc(TConfig& config) noexcept {
    config.crc32 = midismith::config::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&config),
                                                   offsetof(TConfig, crc32));
  }
};

}  // namespace midismith::config
