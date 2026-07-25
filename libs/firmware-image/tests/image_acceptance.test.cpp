#if defined(UNIT_TESTS)

#include "firmware-image/image_acceptance.hpp"

#include <catch2/catch_test_macros.hpp>

#include "firmware-image/image_header.hpp"
#include "firmware-image/product_id.hpp"

namespace {

using midismith::firmware_image::EvaluateImageAcceptance;
using midismith::firmware_image::ImageAcceptance;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::ProductId;
using midismith::firmware_image::TargetConstraints;

constexpr std::uint32_t kApplicationSlotAddress = 0x08100000;
constexpr std::uint32_t kApplicationSlotSizeBytes = 384 * 1024;
constexpr std::uint16_t kSupportedProtocolVersion = 3;

TargetConstraints MakeAdcBoardConstraints() {
  return TargetConstraints{ProductId::kAdcBoard, kApplicationSlotAddress, kApplicationSlotSizeBytes,
                           kSupportedProtocolVersion};
}

ImageHeader MakeAcceptableAdcBoardHeader() {
  ImageHeader header;
  header.product_id = ProductId::kAdcBoard;
  header.payload_size_bytes = 125136;
  header.load_address = kApplicationSlotAddress;
  header.min_compatible_protocol_version = kSupportedProtocolVersion;
  return header;
}

}  // namespace

TEST_CASE("The EvaluateImageAcceptance function") {
  SECTION("When the image matches every target constraint") {
    SECTION("Should accept it") {
      const auto acceptance =
          EvaluateImageAcceptance(MakeAcceptableAdcBoardHeader(), MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kAccepted);
    }
  }

  SECTION("When the image was built for the other board") {
    SECTION("Should reject it so a main-board image never lands on an ADC board") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.product_id = ProductId::kMainBoard;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kProductMismatch);
    }
  }

  SECTION("When the product is unknown to this firmware") {
    SECTION("Should reject it even if the target expects an unknown product") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.product_id = ProductId::kUnknown;
      TargetConstraints constraints = MakeAdcBoardConstraints();
      constraints.expected_product_id = ProductId::kUnknown;

      const auto acceptance = EvaluateImageAcceptance(header, constraints);

      REQUIRE(acceptance == ImageAcceptance::kProductMismatch);
    }
  }

  SECTION("When the image was linked for a different address") {
    SECTION("Should reject it rather than copy it where it cannot run") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.load_address = 0x08000000;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kLoadAddressMismatch);
    }
  }

  SECTION("When the payload is empty") {
    SECTION("Should reject it rather than install an image that would never boot") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.payload_size_bytes = 0;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kPayloadEmpty);
    }
  }

  SECTION("When the payload exactly fills the application slot") {
    SECTION("Should accept it because the slot size is the last usable size") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.payload_size_bytes = kApplicationSlotSizeBytes;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kAccepted);
    }
  }

  SECTION("When the payload is one byte past the application slot") {
    SECTION("Should reject it before any sector is erased") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.payload_size_bytes = kApplicationSlotSizeBytes + 1;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kPayloadTooLarge);
    }
  }

  SECTION("When the image needs a newer protocol than this firmware speaks") {
    SECTION("Should reject it so a board is never cut off from the rest of the bus") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.min_compatible_protocol_version = kSupportedProtocolVersion + 1;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kProtocolTooRecent);
    }
  }

  SECTION("When the image was built against an older protocol") {
    SECTION("Should accept it because a newer peer still understands older images") {
      ImageHeader header = MakeAcceptableAdcBoardHeader();
      header.min_compatible_protocol_version = kSupportedProtocolVersion - 1;

      const auto acceptance = EvaluateImageAcceptance(header, MakeAdcBoardConstraints());

      REQUIRE(acceptance == ImageAcceptance::kAccepted);
    }
  }
}

#endif
