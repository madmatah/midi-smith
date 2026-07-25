#if defined(UNIT_TESTS)

#include "firmware-image/image_installability.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <vector>

#include "firmware-image/image_header.hpp"
#include "image_fixtures.hpp"
#include "product-id/product_id.hpp"

namespace {

using midismith::firmware_image::EvaluateImageInstallability;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::ImageInstallability;
using midismith::firmware_image::kFlashWordSizeBytes;
using midismith::firmware_image::TargetConstraints;
using midismith::firmware_image::test::kApplicationSlotSizeBytes;
using midismith::firmware_image::test::kSupportedProtocolVersion;
using midismith::firmware_image::test::MakeAdcBoardConstraints;
using midismith::firmware_image::test::MakeHeaderDescribing;
using midismith::firmware_image::test::MakePayload;
using midismith::product_id::ProductId;

constexpr std::size_t kSamplePayloadFlashWordCount = 4;

}  // namespace

TEST_CASE("The EvaluateImageInstallability function") {
  const std::vector<std::uint8_t> payload = MakePayload(kSamplePayloadFlashWordCount);
  const ImageHeader matching_header = MakeHeaderDescribing(payload);

  SECTION("When the image matches every target constraint and its payload is intact") {
    SECTION("Should declare it installable") {
      const auto installability =
          EvaluateImageInstallability(matching_header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kInstallable);
    }
  }

  SECTION("When a single payload byte was corrupted in transit") {
    SECTION("Should reject it before any sector is erased") {
      std::vector<std::uint8_t> corrupted_payload = payload;
      corrupted_payload[corrupted_payload.size() / 2] ^= 0x01;

      const auto installability = EvaluateImageInstallability(matching_header, corrupted_payload,
                                                              MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadChecksumMismatch);
    }
  }

  SECTION("When the payload is intact but the header announces another checksum") {
    SECTION("Should reject it, because the header is what the bootloader will trust") {
      ImageHeader header = matching_header;
      header.payload_crc32 ^= 0x0000FFFFu;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadChecksumMismatch);
    }
  }

  SECTION("When the image was built for the other board") {
    SECTION("Should reject it so a main-board image never lands on an ADC board") {
      ImageHeader header = matching_header;
      header.product_id = ProductId::kMainBoard;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kProductMismatch);
    }
  }

  SECTION("When the product is unknown to this firmware") {
    SECTION("Should reject it even if the target expects an unknown product") {
      ImageHeader header = matching_header;
      header.product_id = ProductId::kUnknown;
      TargetConstraints constraints = MakeAdcBoardConstraints();
      constraints.expected_product_id = ProductId::kUnknown;

      const auto installability = EvaluateImageInstallability(header, payload, constraints);

      REQUIRE(installability == ImageInstallability::kProductMismatch);
    }
  }

  SECTION("When the image was linked for a different address") {
    SECTION("Should reject it rather than copy it where it cannot run") {
      ImageHeader header = matching_header;
      header.load_address = 0x08000000;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kLoadAddressMismatch);
    }
  }

  SECTION("When the payload is empty") {
    SECTION("Should reject it rather than install an image that would never boot") {
      ImageHeader header = matching_header;
      header.payload_size_bytes = 0;

      const auto installability = EvaluateImageInstallability(
          header, std::span<const std::uint8_t>{}, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadEmpty);
    }
  }

  SECTION("When the payload exactly fills the application slot") {
    SECTION("Should declare it installable, because the slot size is the last usable size") {
      const std::vector<std::uint8_t> slot_sized_payload =
          MakePayload(kApplicationSlotSizeBytes / kFlashWordSizeBytes);

      const auto installability = EvaluateImageInstallability(
          MakeHeaderDescribing(slot_sized_payload), slot_sized_payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kInstallable);
    }
  }

  SECTION("When the payload is one byte past the application slot") {
    SECTION("Should reject it before any sector is erased") {
      ImageHeader header = matching_header;
      header.payload_size_bytes = kApplicationSlotSizeBytes + 1;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadTooLarge);
    }
  }

  SECTION("When the target was never given a slot size") {
    SECTION("Should reject the image rather than install it into a slot of unknown length") {
      TargetConstraints constraints = MakeAdcBoardConstraints();
      constraints.maximum_payload_size_bytes = 0;

      const auto installability =
          EvaluateImageInstallability(matching_header, payload, constraints);

      REQUIRE(installability == ImageInstallability::kPayloadTooLarge);
    }
  }

  SECTION("When the target was default constructed and configured with nothing") {
    SECTION("Should reject on the first constraint rather than fall through to installable") {
      const auto installability =
          EvaluateImageInstallability(matching_header, payload, TargetConstraints{});

      REQUIRE(installability == ImageInstallability::kProductMismatch);
    }
  }

  SECTION("When the payload is not a whole number of flash words") {
    SECTION("Should reject it, because the H743 cannot program a partial word") {
      ImageHeader header = matching_header;
      header.payload_size_bytes = kApplicationSlotSizeBytes - 1;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadMisaligned);
    }
  }

  SECTION("When fewer payload bytes were handed over than the header announces") {
    SECTION("Should reject it rather than checksum a short buffer") {
      const std::span<const std::uint8_t> short_payload =
          std::span<const std::uint8_t>{payload}.first(payload.size() - kFlashWordSizeBytes);

      const auto installability =
          EvaluateImageInstallability(matching_header, short_payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kPayloadTruncated);
    }
  }

  SECTION("When the image needs a newer protocol than this firmware speaks") {
    SECTION("Should reject it so a board is never cut off from the rest of the bus") {
      ImageHeader header = matching_header;
      header.min_compatible_protocol_version = kSupportedProtocolVersion + 1;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kProtocolTooRecent);
    }
  }

  SECTION("When the image was built against an older protocol") {
    SECTION("Should accept it because a newer peer still understands older images") {
      ImageHeader header = MakeHeaderDescribing(payload);
      header.min_compatible_protocol_version = kSupportedProtocolVersion - 1;

      const auto installability =
          EvaluateImageInstallability(header, payload, MakeAdcBoardConstraints());

      REQUIRE(installability == ImageInstallability::kInstallable);
    }
  }
}

#endif
