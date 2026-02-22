#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace midismith::adc_board::domain::config {

inline constexpr std::size_t kConfigSizeBytes = 256;
inline constexpr std::size_t kConfigHeaderSizeBytes = 8;
inline constexpr std::size_t kConfigCrcSizeBytes = sizeof(std::uint32_t);

template <typename T, std::uint32_t Magic, std::uint16_t Version>
struct alignas(32) [[gnu::packed]] StorableConfig {
  static_assert(std::is_trivially_copyable_v<T>, "Storable config data must be trivially copyable");
  static_assert(std::is_standard_layout_v<T>, "Storable config data must use standard layout");
  static_assert(kConfigSizeBytes >= kConfigHeaderSizeBytes + sizeof(T) + kConfigCrcSizeBytes,
                "Storable config data is too large");
  static_assert((kConfigHeaderSizeBytes % alignof(T)) == 0,
                "Storable config data alignment is incompatible with packed header");

  static constexpr std::uint32_t kMagicNumber = Magic;
  static constexpr std::uint16_t kVersion = Version;
  static constexpr std::size_t kPaddingSizeBytes =
      kConfigSizeBytes - kConfigHeaderSizeBytes - sizeof(T) - kConfigCrcSizeBytes;

  std::uint32_t magic_number;
  std::uint16_t version;
  std::uint16_t reserved_header;
  T data;
  std::uint8_t padding[kPaddingSizeBytes];
  std::uint32_t crc32;
};

template <typename T, std::uint32_t Magic, std::uint16_t Version>
struct StorableConfigLayout {
  using ConfigType = StorableConfig<T, Magic, Version>;
  static constexpr bool kValue =
      sizeof(ConfigType) == kConfigSizeBytes && alignof(ConfigType) == 32 &&
      sizeof(ConfigType) % 32 == 0 &&
      offsetof(ConfigType, crc32) + sizeof(std::uint32_t) == kConfigSizeBytes;
};

template <typename T, std::uint32_t Magic, std::uint16_t Version>
inline constexpr bool kStorableConfigLayoutValid = StorableConfigLayout<T, Magic, Version>::kValue;

}  // namespace midismith::adc_board::domain::config
