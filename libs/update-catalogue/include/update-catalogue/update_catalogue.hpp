#pragma once

#include <cstdint>
#include <string_view>

#include "firmware-image/image_header.hpp"
#include "product-id/product_id.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::update_catalogue {

inline constexpr std::string_view kImageDirectory = "/midismith";
inline constexpr std::string_view kMainBoardImagePath = "/midismith/main-board.msfw";
inline constexpr std::string_view kAdcBoardImagePath = "/midismith/adc-board.msfw";

static_assert(kMainBoardImagePath.size() <= kMaxImagePathLengthBytes &&
                  kAdcBoardImagePath.size() <= kMaxImagePathLengthBytes,
              "an image source rejects a longer path, and that rejection is indistinguishable from "
              "a missing file: a renamed image would be reported absent with nothing saying why");


[[nodiscard]] std::string_view ImagePathFor(product_id::ProductId product) noexcept;

enum class CatalogueStatus : std::uint8_t {
  kImageAvailable = 0,
  kNoImageOnCard,
  kUnreadable,
  kNotAnImage,
  kWrongProduct,
};

struct CatalogueEntry {
  CatalogueStatus status = CatalogueStatus::kNoImageOnCard;
  firmware_image::ImageHeader header{};
  std::uint32_t container_size_bytes = 0;

  [[nodiscard]] constexpr bool has_image() const noexcept {
    return status == CatalogueStatus::kImageAvailable;
  }
};

enum class UpdateNeed : std::uint8_t {
  kUpToDate = 0,
  kUpdateAvailable,
  kInstalledVersionUnknown,
  kNoImage,
  kImageUnusable,
};

class UpdateCatalogue {
 public:
  explicit UpdateCatalogue(ImageSourceRequirements& source) noexcept : source_(source) {}

  [[nodiscard]] CatalogueEntry Lookup(product_id::ProductId product) noexcept;

 private:
  ImageSourceRequirements& source_;
};

[[nodiscard]] UpdateNeed EvaluateUpdateNeed(const CatalogueEntry& entry,
                                            std::string_view installed_version) noexcept;

}  // namespace midismith::update_catalogue
