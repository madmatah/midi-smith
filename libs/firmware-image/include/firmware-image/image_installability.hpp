#pragma once

#include <cstdint>
#include <span>

#include "firmware-image/image_header.hpp"
#include "product-id/product_id.hpp"

namespace midismith::firmware_image {

using product_id::ProductId;

struct TargetConstraints {
  ProductId expected_product_id = ProductId::kUnknown;
  std::uint32_t expected_load_address = 0;
  std::uint32_t maximum_payload_size_bytes = 0;
  std::uint16_t supported_protocol_version = 0;
};

enum class ImageInstallability : std::uint8_t {
  kInstallable = 0,
  kProductMismatch,
  kLoadAddressMismatch,
  kPayloadEmpty,
  kPayloadTooLarge,
  kPayloadMisaligned,
  kPayloadTruncated,
  kProtocolTooRecent,
  kPayloadChecksumMismatch,
};

[[nodiscard]] ImageInstallability EvaluateImageInstallability(
    const ImageHeader& header, std::span<const std::uint8_t> payload,
    const TargetConstraints& constraints) noexcept;

}  // namespace midismith::firmware_image
