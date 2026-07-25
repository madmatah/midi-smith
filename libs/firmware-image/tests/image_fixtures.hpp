#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "checksum/crc32.hpp"
#include "firmware-image/image_header.hpp"
#include "firmware-image/image_installability.hpp"
#include "product-id/product_id.hpp"

namespace midismith::firmware_image::test {

inline constexpr std::uint32_t kApplicationSlotAddress = 0x08100000;
inline constexpr std::uint32_t kApplicationSlotSizeBytes = 384 * 1024;
inline constexpr std::uint16_t kSupportedProtocolVersion = 3;
inline constexpr std::uint32_t kSampleAdcBoardPayloadSizeBytes = 125344;

inline constexpr std::string_view kSampleVersionString = "v1.3.0-4-gab12cd34ef";
inline constexpr std::string_view kSampleBuildDate = "2026-07-25T10:31:07+02:00";

template <std::size_t kCapacity>
std::array<char, kCapacity> MakeTextField(std::string_view text) {
  std::array<char, kCapacity> field{};
  const std::size_t copied_length_bytes = std::min(text.size(), kCapacity - 1);
  std::copy_n(text.begin(), copied_length_bytes, field.begin());
  return field;
}

inline TargetConstraints MakeAdcBoardConstraints() {
  return TargetConstraints{.expected_product_id = ProductId::kAdcBoard,
                           .expected_load_address = kApplicationSlotAddress,
                           .maximum_payload_size_bytes = kApplicationSlotSizeBytes,
                           .supported_protocol_version = kSupportedProtocolVersion};
}

inline ImageHeader MakeAdcBoardHeader() {
  ImageHeader header;
  header.product_id = ProductId::kAdcBoard;
  header.payload_size_bytes = kSampleAdcBoardPayloadSizeBytes;
  header.payload_crc32 = 0xDEADBEEF;
  header.load_address = kApplicationSlotAddress;
  header.min_compatible_protocol_version = kSupportedProtocolVersion;
  header.version_string = MakeTextField<kVersionStringCapacity>(kSampleVersionString);
  header.build_date = MakeTextField<kBuildDateCapacity>(kSampleBuildDate);
  return header;
}

inline std::vector<std::uint8_t> MakePayload(std::size_t flash_word_count) {
  std::vector<std::uint8_t> payload(flash_word_count * kFlashWordSizeBytes);
  for (std::size_t index = 0; index < payload.size(); ++index) {
    payload[index] = static_cast<std::uint8_t>(index * 7 + 1);
  }
  return payload;
}

inline ImageHeader MakeHeaderDescribing(std::span<const std::uint8_t> payload) {
  ImageHeader header = MakeAdcBoardHeader();
  header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
  header.payload_crc32 = midismith::checksum::ComputeCrc32(payload);
  return header;
}

}  // namespace midismith::firmware_image::test
