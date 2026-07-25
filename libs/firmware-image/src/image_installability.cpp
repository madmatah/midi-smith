#include "firmware-image/image_installability.hpp"

#include "checksum/crc32.hpp"

namespace midismith::firmware_image {

ImageInstallability EvaluateImageInstallability(const ImageHeader& header,
                                                std::span<const std::uint8_t> payload,
                                                const TargetConstraints& constraints) noexcept {
  if (header.product_id != constraints.expected_product_id ||
      header.product_id == ProductId::kUnknown) {
    return ImageInstallability::kProductMismatch;
  }

  if (header.load_address != constraints.expected_load_address) {
    return ImageInstallability::kLoadAddressMismatch;
  }

  if (header.payload_size_bytes == 0) {
    return ImageInstallability::kPayloadEmpty;
  }

  if (header.payload_size_bytes > constraints.maximum_payload_size_bytes) {
    return ImageInstallability::kPayloadTooLarge;
  }

  if ((header.payload_size_bytes % kFlashWordSizeBytes) != 0) {
    return ImageInstallability::kPayloadMisaligned;
  }

  if (payload.size() != header.payload_size_bytes) {
    return ImageInstallability::kPayloadTruncated;
  }

  if (header.min_compatible_protocol_version > constraints.supported_protocol_version) {
    return ImageInstallability::kProtocolTooRecent;
  }

  if (midismith::checksum::ComputeCrc32(payload) != header.payload_crc32) {
    return ImageInstallability::kPayloadChecksumMismatch;
  }

  return ImageInstallability::kInstallable;
}

}  // namespace midismith::firmware_image
