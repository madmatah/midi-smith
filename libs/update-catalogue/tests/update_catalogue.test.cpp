#if defined(UNIT_TESTS)

#include "update-catalogue/update_catalogue.hpp"

#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <map>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "firmware-image/image_header.hpp"
#include "product-id/product_id.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace {

using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::kVersionStringCapacity;
using midismith::product_id::ProductId;
using midismith::update_catalogue::CatalogueEntry;
using midismith::update_catalogue::CatalogueStatus;
using midismith::update_catalogue::EvaluateUpdateNeed;
using midismith::update_catalogue::ImagePathFor;
using midismith::update_catalogue::ImageSourceRequirements;
using midismith::update_catalogue::kAdcBoardImagePath;
using midismith::update_catalogue::kMainBoardImagePath;
using midismith::update_catalogue::UpdateCatalogue;
using midismith::update_catalogue::UpdateNeed;

constexpr std::uint32_t kApplicationLoadAddress = 0x08100000;
constexpr std::uint32_t kSamplePayloadSizeBytes = 125344;
constexpr std::string_view kInstalledVersion = "v1.2.0-2-gaabbccddee";
constexpr std::string_view kNewerVersion = "v1.3.0-4-gab12cd34ef";

std::array<char, kVersionStringCapacity> VersionField(std::string_view text) {
  std::array<char, kVersionStringCapacity> field{};
  std::copy_n(text.begin(), std::min(text.size(), kVersionStringCapacity - 1), field.begin());
  return field;
}

std::vector<std::uint8_t> MakeContainer(ProductId product, std::string_view version,
                                        std::uint32_t payload_size = kSamplePayloadSizeBytes) {
  ImageHeader header;
  header.product_id = product;
  header.payload_size_bytes = payload_size;
  header.payload_crc32 = 0xABCDEF01;
  header.load_address = kApplicationLoadAddress;
  header.version_string = VersionField(version);

  std::vector<std::uint8_t> container(kImageHeaderSizeBytes);
  REQUIRE(header.Serialize(container).has_value());
  container.resize(kImageHeaderSizeBytes + payload_size, 0x5A);
  return container;
}

class FakeCard final : public ImageSourceRequirements {
 public:
  void Place(std::string_view path, std::vector<std::uint8_t> content) {
    files_[std::string{path}] = std::move(content);
  }

  void MakeUnreadable(std::string_view path) {
    unreadable_.insert(std::string{path});
  }

  std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept override {
    const auto found = files_.find(std::string{path});
    if (found == files_.end()) {
      return std::nullopt;
    }
    return static_cast<std::uint32_t>(found->second.size());
  }

  std::optional<std::size_t> ReadAt(std::string_view path, std::uint32_t offset_bytes,
                                    std::span<std::uint8_t> out) noexcept override {
    if (unreadable_.contains(std::string{path})) {
      return std::nullopt;
    }
    const auto found = files_.find(std::string{path});
    if (found == files_.end() || offset_bytes > found->second.size()) {
      return std::nullopt;
    }
    const std::size_t available = found->second.size() - offset_bytes;
    const std::size_t copied = std::min(available, out.size());
    std::copy_n(found->second.begin() + offset_bytes, copied, out.begin());
    return copied;
  }

 private:
  std::map<std::string, std::vector<std::uint8_t>> files_;
  std::set<std::string> unreadable_;
};

}  // namespace

TEST_CASE("A board whose running version is unknown is never called up to date") {
  SECTION(
      "Only the main board reads its own version string; an adc board reports its own over CAN, "
      "and an image whose version field is blank would otherwise compare equal to that ignorance") {
    SECTION("When the caller cannot state the installed version") {
      SECTION("Should say the comparison is impossible rather than claim a match") {
        CatalogueEntry entry;
        entry.status = CatalogueStatus::kImageAvailable;
        entry.container_size_bytes = kImageHeaderSizeBytes + kSamplePayloadSizeBytes;
        entry.header.payload_size_bytes = kSamplePayloadSizeBytes;

        REQUIRE(EvaluateUpdateNeed(entry, std::string_view{}) ==
                UpdateNeed::kInstalledVersionUnknown);
      }
    }
  }
}

TEST_CASE("The ImagePathFor function") {
  SECTION("When asked for a board the instrument carries") {
    SECTION("Should name the file the operator copies onto the card") {
      REQUIRE(ImagePathFor(ProductId::kMainBoard) == "/midismith/main-board.msfw");
      REQUIRE(ImagePathFor(ProductId::kAdcBoard) == "/midismith/adc-board.msfw");
    }
  }

  SECTION("When asked for an unknown product") {
    SECTION("Should name no file, so nothing is ever looked up by accident") {
      REQUIRE(ImagePathFor(ProductId::kUnknown).empty());
    }
  }
}

TEST_CASE("The UpdateCatalogue class") {
  SECTION("The Lookup() method") {
    SECTION("When the card carries an image for the board") {
      SECTION("Should report it available, with the header the operator can be shown") {
        FakeCard card;
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kNewerVersion));
        UpdateCatalogue catalogue{card};

        const auto entry = catalogue.Lookup(ProductId::kMainBoard);

        REQUIRE(entry.status == CatalogueStatus::kImageAvailable);
        REQUIRE(entry.has_image());
        REQUIRE(std::string_view(entry.header.version_string.data()) == kNewerVersion);
      }
    }

    SECTION("When the card carries no image for the board") {
      SECTION("Should report none rather than fail, an absent file is a normal answer") {
        FakeCard card;
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kNewerVersion));
        UpdateCatalogue catalogue{card};

        REQUIRE(catalogue.Lookup(ProductId::kAdcBoard).status == CatalogueStatus::kNoImageOnCard);
      }
    }

    SECTION("When the file exists but cannot be read") {
      SECTION("Should say so, because a failing card is not the same as an absent image") {
        FakeCard card;
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kNewerVersion));
        card.MakeUnreadable(kMainBoardImagePath);
        UpdateCatalogue catalogue{card};

        REQUIRE(catalogue.Lookup(ProductId::kMainBoard).status == CatalogueStatus::kUnreadable);
      }
    }

    SECTION("When the file is not a firmware container at all") {
      SECTION("Should reject it, so an unrelated file dropped on the card is harmless") {
        FakeCard card;
        card.Place(kMainBoardImagePath, std::vector<std::uint8_t>(kImageHeaderSizeBytes, 0x00));
        UpdateCatalogue catalogue{card};

        REQUIRE(catalogue.Lookup(ProductId::kMainBoard).status == CatalogueStatus::kNotAnImage);
      }
    }

    SECTION("When the file is shorter than a header") {
      SECTION("Should report it unreadable rather than parse whatever it holds") {
        FakeCard card;
        card.Place(kMainBoardImagePath, std::vector<std::uint8_t>(kImageHeaderSizeBytes - 1, 0xFF));
        UpdateCatalogue catalogue{card};

        REQUIRE(catalogue.Lookup(ProductId::kMainBoard).status == CatalogueStatus::kUnreadable);
      }
    }

    SECTION("When an ADC image was copied under the main board's name") {
      SECTION("Should refuse it, the file name is not what decides the target") {
        FakeCard card;
        card.Place(kMainBoardImagePath, MakeContainer(ProductId::kAdcBoard, kNewerVersion));
        UpdateCatalogue catalogue{card};

        REQUIRE(catalogue.Lookup(ProductId::kMainBoard).status == CatalogueStatus::kWrongProduct);
      }
    }
  }
}

TEST_CASE("The EvaluateUpdateNeed function") {
  FakeCard card;
  UpdateCatalogue catalogue{card};

  SECTION("When the card offers a build other than the one running") {
    SECTION("Should report an update is available") {
      card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kNewerVersion));

      const auto need =
          EvaluateUpdateNeed(catalogue.Lookup(ProductId::kMainBoard), kInstalledVersion);

      REQUIRE(need == UpdateNeed::kUpdateAvailable);
    }
  }

  SECTION("When the card offers the very build already running") {
    SECTION("Should report nothing to do, so a card left in the slot never reflashes on boot") {
      card.Place(kMainBoardImagePath, MakeContainer(ProductId::kMainBoard, kInstalledVersion));

      const auto need =
          EvaluateUpdateNeed(catalogue.Lookup(ProductId::kMainBoard), kInstalledVersion);

      REQUIRE(need == UpdateNeed::kUpToDate);
    }
  }

  SECTION("When the container is shorter than the payload its header announces") {
    SECTION("Should refuse it, a copy interrupted mid-write must never be offered") {
      auto truncated = MakeContainer(ProductId::kMainBoard, kNewerVersion);
      truncated.resize(truncated.size() - 1);
      card.Place(kMainBoardImagePath, std::move(truncated));

      const auto need =
          EvaluateUpdateNeed(catalogue.Lookup(ProductId::kMainBoard), kInstalledVersion);

      REQUIRE(need == UpdateNeed::kImageUnusable);
    }
  }

  SECTION("When the card carries no image for the board") {
    SECTION("Should report no image, which is not an error the operator must act on") {
      const auto need =
          EvaluateUpdateNeed(catalogue.Lookup(ProductId::kAdcBoard), kInstalledVersion);

      REQUIRE(need == UpdateNeed::kNoImage);
    }
  }

  SECTION("When the image targets another board") {
    SECTION("Should report it unusable rather than silently skip it") {
      card.Place(kMainBoardImagePath, MakeContainer(ProductId::kAdcBoard, kNewerVersion));

      const auto need =
          EvaluateUpdateNeed(catalogue.Lookup(ProductId::kMainBoard), kInstalledVersion);

      REQUIRE(need == UpdateNeed::kImageUnusable);
    }
  }
}

#endif
