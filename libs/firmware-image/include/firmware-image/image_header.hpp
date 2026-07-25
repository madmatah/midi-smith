#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "firmware-image/product_id.hpp"

namespace midismith::firmware_image {

inline constexpr std::size_t kImageHeaderSizeBytes = 96;
inline constexpr std::size_t kVersionStringCapacity = 32;
inline constexpr std::size_t kBuildDateCapacity = 32;
inline constexpr std::uint16_t kSupportedFormatVersion = 1;
inline constexpr std::array<std::uint8_t, 4> kImageMagic = {'M', 'S', 'F', 'W'};
inline constexpr std::size_t kFlashWordSizeBytes = 32;

static_assert(kImageHeaderSizeBytes % kFlashWordSizeBytes == 0,
              "The payload follows the header in flash and must stay flash-word aligned");

struct ImageHeader {
  std::uint16_t format_version = kSupportedFormatVersion;
  ProductId product_id = ProductId::kUnknown;
  std::uint32_t payload_size_bytes = 0;
  std::uint32_t payload_crc32 = 0;
  std::uint32_t load_address = 0;
  std::uint16_t min_compatible_protocol_version = 0;
  std::array<char, kVersionStringCapacity> version_string{};
  std::array<char, kBuildDateCapacity> build_date{};

  bool operator==(const ImageHeader&) const = default;

  [[nodiscard]] std::optional<std::size_t> Serialize(
      std::span<std::uint8_t> out_buffer) const noexcept;
};

enum class ImageHeaderStatus : std::uint8_t {
  kValid = 0,
  kBufferTooSmall,
  kMagicMismatch,
  kUnsupportedFormatVersion,
  kHeaderChecksumMismatch,
};

struct ImageHeaderParseResult {
  ImageHeaderStatus status = ImageHeaderStatus::kBufferTooSmall;
  ImageHeader header{};

  [[nodiscard]] constexpr bool is_valid() const noexcept {
    return status == ImageHeaderStatus::kValid;
  }
};

[[nodiscard]] ImageHeaderParseResult ParseImageHeader(
    std::span<const std::uint8_t> container) noexcept;

[[nodiscard]] std::optional<std::span<const std::uint8_t>> ContainerPayload(
    const ImageHeader& header, std::span<const std::uint8_t> container) noexcept;

}  // namespace midismith::firmware_image
