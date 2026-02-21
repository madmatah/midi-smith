#ifndef DOMAIN_CONFIG_USER_CONFIG_HPP
#define DOMAIN_CONFIG_USER_CONFIG_HPP

#include <cstddef>
#include <cstdint>

namespace domain::config {

inline constexpr std::uint32_t kConfigMagicNumber = 0x43464721u;
inline constexpr std::uint16_t kConfigVersion = 1;
inline constexpr std::size_t kConfigSizeBytes = 256;
inline constexpr std::size_t kConfigHeaderSizeBytes = 8;
inline constexpr std::size_t kConfigCrcSizeBytes = sizeof(std::uint32_t);
inline constexpr std::uint8_t kMinBoardId = 1;
inline constexpr std::uint8_t kMaxBoardId = 8;
inline constexpr std::uint8_t kDefaultBoardId = 1;

struct alignas(32) UserConfig {
  std::uint32_t magic_number;
  std::uint16_t version;
  std::uint16_t reserved_header;

  struct Data {
    std::uint8_t can_board_id;
  } data;

  std::uint8_t
      padding[kConfigSizeBytes - kConfigHeaderSizeBytes - sizeof(Data) - kConfigCrcSizeBytes];

  std::uint32_t crc32;
};

static_assert(sizeof(UserConfig) == kConfigSizeBytes);
static_assert(alignof(UserConfig) == 32);
static_assert(sizeof(UserConfig) % 32 == 0);
static_assert(offsetof(UserConfig, crc32) + sizeof(std::uint32_t) == kConfigSizeBytes);

UserConfig DefaultConfig() noexcept;

}  // namespace domain::config

#endif  // DOMAIN_CONFIG_USER_CONFIG_HPP
