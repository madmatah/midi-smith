#include "firmware-image/image_acceptance.hpp"

namespace midismith::firmware_image {

ImageAcceptance EvaluateImageAcceptance(const ImageHeader& header,
                                        const TargetConstraints& constraints) noexcept {
  if (header.product_id != constraints.expected_product_id ||
      header.product_id == ProductId::kUnknown) {
    return ImageAcceptance::kProductMismatch;
  }

  if (header.load_address != constraints.expected_load_address) {
    return ImageAcceptance::kLoadAddressMismatch;
  }

  if (header.payload_size_bytes == 0) {
    return ImageAcceptance::kPayloadEmpty;
  }

  if (header.payload_size_bytes > constraints.maximum_payload_size_bytes) {
    return ImageAcceptance::kPayloadTooLarge;
  }

  if (header.min_compatible_protocol_version > constraints.supported_protocol_version) {
    return ImageAcceptance::kProtocolTooRecent;
  }

  return ImageAcceptance::kAccepted;
}

}  // namespace midismith::firmware_image
