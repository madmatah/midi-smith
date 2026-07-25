#pragma once

#include <cstdint>

namespace midismith::firmware_image {

enum class ProductId : std::uint16_t {
  kUnknown = 0,
  kMainBoard = 1,
  kAdcBoard = 2,
};

[[nodiscard]] constexpr ProductId MakeProductId(std::uint16_t raw_value) noexcept {
  switch (static_cast<ProductId>(raw_value)) {
    case ProductId::kMainBoard:
      return ProductId::kMainBoard;
    case ProductId::kAdcBoard:
      return ProductId::kAdcBoard;
    case ProductId::kUnknown:
    default:
      return ProductId::kUnknown;
  }
}

}  // namespace midismith::firmware_image
