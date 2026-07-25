#pragma once

#include <cstdint>

#include "firmware-image/image_header.hpp"
#include "firmware-image/product_id.hpp"

namespace midismith::firmware_image {

struct TargetConstraints {
  ProductId expected_product_id = ProductId::kUnknown;
  std::uint32_t expected_load_address = 0;
  std::uint32_t maximum_payload_size_bytes = 0;
  std::uint16_t supported_protocol_version = 0;
};

enum class ImageAcceptance : std::uint8_t {
  kAccepted = 0,
  kProductMismatch,
  kLoadAddressMismatch,
  kPayloadEmpty,
  kPayloadTooLarge,
  kProtocolTooRecent,
};

[[nodiscard]] ImageAcceptance EvaluateImageAcceptance(
    const ImageHeader& header, const TargetConstraints& constraints) noexcept;

}  // namespace midismith::firmware_image
