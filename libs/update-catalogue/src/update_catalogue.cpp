#include "update-catalogue/update_catalogue.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::update_catalogue {

std::string_view ImagePathFor(product_id::ProductId product) noexcept {
  switch (product) {
    case product_id::ProductId::kMainBoard:
      return kMainBoardImagePath;
    case product_id::ProductId::kAdcBoard:
      return kAdcBoardImagePath;
    case product_id::ProductId::kUnknown:
    default:
      return {};
  }
}

CatalogueEntry UpdateCatalogue::Lookup(product_id::ProductId product) noexcept {
  CatalogueEntry entry;

  const std::string_view path = ImagePathFor(product);
  if (path.empty()) {
    entry.status = CatalogueStatus::kNoImageOnCard;
    return entry;
  }

  const auto container_size = source_.SizeOf(path);
  if (!container_size.has_value()) {
    entry.status = CatalogueStatus::kNoImageOnCard;
    return entry;
  }
  entry.container_size_bytes = *container_size;

  std::array<std::uint8_t, firmware_image::kImageHeaderSizeBytes> header_bytes{};
  const auto read_length = source_.ReadAt(path, 0, header_bytes);
  if (!read_length.has_value() || *read_length != header_bytes.size()) {
    entry.status = CatalogueStatus::kUnreadable;
    return entry;
  }

  const auto parsed = firmware_image::ParseImageHeader(header_bytes);
  if (!parsed.is_valid()) {
    entry.status = CatalogueStatus::kNotAnImage;
    return entry;
  }

  if (parsed.header.product_id != product) {
    entry.status = CatalogueStatus::kWrongProduct;
    entry.header = parsed.header;
    return entry;
  }

  entry.status = CatalogueStatus::kImageAvailable;
  entry.header = parsed.header;
  return entry;
}

UpdateNeed EvaluateUpdateNeed(const CatalogueEntry& entry,
                              std::string_view installed_version) noexcept {
  switch (entry.status) {
    case CatalogueStatus::kNoImageOnCard:
      return UpdateNeed::kNoImage;
    case CatalogueStatus::kUnreadable:
    case CatalogueStatus::kNotAnImage:
    case CatalogueStatus::kWrongProduct:
      return UpdateNeed::kImageUnusable;
    case CatalogueStatus::kImageAvailable:
      break;
  }

  const std::uint32_t announced_payload_bytes = entry.header.payload_size_bytes;
  const std::uint32_t expected_container_bytes =
      announced_payload_bytes + firmware_image::kImageHeaderSizeBytes;
  if (entry.container_size_bytes != expected_container_bytes) {
    return UpdateNeed::kImageUnusable;
  }

  const std::string_view offered_version{entry.header.version_string.data()};
  if (offered_version == installed_version) {
    return UpdateNeed::kUpToDate;
  }

  return UpdateNeed::kUpdateAvailable;
}

}  // namespace midismith::update_catalogue
