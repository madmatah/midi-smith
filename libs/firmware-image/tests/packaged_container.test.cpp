#if defined(UNIT_TESTS)

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <string_view>

#include "firmware-image/image_header.hpp"
#include "firmware-image/image_installability.hpp"
#include "firmware-image/product_id.hpp"
#include "image_fixtures.hpp"

namespace {

using midismith::firmware_image::ContainerPayload;
using midismith::firmware_image::EvaluateImageInstallability;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::ImageHeaderStatus;
using midismith::firmware_image::ImageInstallability;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::ParseImageHeader;
using midismith::firmware_image::ProductId;
using midismith::firmware_image::test::kSampleBuildDate;
using midismith::firmware_image::test::kSampleVersionString;
using midismith::firmware_image::test::MakeAdcBoardConstraints;
using midismith::firmware_image::test::MakeTextField;

constexpr std::size_t kGoldenContainerSizeBytes = 160;
constexpr std::string_view kGoldenPayloadText = "midi-smith firmware payload golden vector";

constexpr std::array<std::uint8_t, kGoldenContainerSizeBytes> kGoldenContainer = {
    0x4D, 0x53, 0x46, 0x57, 0x01, 0x00, 0x02, 0x00, 0x40, 0x00, 0x00, 0x00, 0xEC, 0xF1, 0x3A, 0xD0,
    0x00, 0x00, 0x10, 0x08, 0x03, 0x00, 0x00, 0x00, 0x76, 0x31, 0x2E, 0x33, 0x2E, 0x30, 0x2D, 0x34,
    0x2D, 0x67, 0x61, 0x62, 0x31, 0x32, 0x63, 0x64, 0x33, 0x34, 0x65, 0x66, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x30, 0x32, 0x36, 0x2D, 0x30, 0x37, 0x2D,
    0x32, 0x35, 0x54, 0x31, 0x30, 0x3A, 0x33, 0x31, 0x3A, 0x30, 0x37, 0x2B, 0x30, 0x32, 0x3A, 0x30,
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x83, 0x4B, 0xFE, 0xD2,
    0x6D, 0x69, 0x64, 0x69, 0x2D, 0x73, 0x6D, 0x69, 0x74, 0x68, 0x20, 0x66, 0x69, 0x72, 0x6D, 0x77,
    0x61, 0x72, 0x65, 0x20, 0x70, 0x61, 0x79, 0x6C, 0x6F, 0x61, 0x64, 0x20, 0x67, 0x6F, 0x6C, 0x64,
    0x65, 0x6E, 0x20, 0x76, 0x65, 0x63, 0x74, 0x6F, 0x72, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

ImageHeader MakeGoldenHeader() {
  ImageHeader header;
  header.product_id = ProductId::kAdcBoard;
  header.payload_size_bytes = kGoldenContainerSizeBytes - kImageHeaderSizeBytes;
  header.payload_crc32 = 0xD03AF1EC;
  header.load_address = 0x08100000;
  header.min_compatible_protocol_version = 3;
  header.version_string =
      MakeTextField<midismith::firmware_image::kVersionStringCapacity>(kSampleVersionString);
  header.build_date =
      MakeTextField<midismith::firmware_image::kBuildDateCapacity>(kSampleBuildDate);
  return header;
}

}  // namespace

TEST_CASE("The .msfw container produced by tools/firmware_packager.py") {
  SECTION("When the firmware that will install it parses the container") {
    SECTION("Should accept it, pinning the byte layout both implementations agree on") {
      REQUIRE(ParseImageHeader(kGoldenContainer).status == ImageHeaderStatus::kValid);
    }

    SECTION("Should recover exactly the header the packaging tool was asked to write") {
      REQUIRE(ParseImageHeader(kGoldenContainer).header == MakeGoldenHeader());
    }

    SECTION("Should carry the git version and commit date the build passed in") {
      const auto parsed = ParseImageHeader(kGoldenContainer);

      REQUIRE(std::string_view(parsed.header.version_string.data()) == kSampleVersionString);
      REQUIRE(std::string_view(parsed.header.build_date.data()) == kSampleBuildDate);
    }
  }

  SECTION("When the firmware serializes the same header back") {
    SECTION("Should emit byte for byte what the packaging tool emitted") {
      std::array<std::uint8_t, kImageHeaderSizeBytes> written{};

      const auto written_length_bytes = MakeGoldenHeader().Serialize(written);

      REQUIRE(written_length_bytes.has_value());
      REQUIRE(std::ranges::equal(
          written, std::span<const std::uint8_t>{kGoldenContainer}.first(kImageHeaderSizeBytes)));
    }
  }

  SECTION("When the payload is taken from the container") {
    SECTION("Should start right after the header and run to the end of the container") {
      const auto parsed = ParseImageHeader(kGoldenContainer);
      const auto payload = ContainerPayload(parsed.header, kGoldenContainer);

      REQUIRE(payload.has_value());
      REQUIRE(payload->size() == kGoldenContainerSizeBytes - kImageHeaderSizeBytes);
      REQUIRE(
          std::ranges::equal(payload->first(kGoldenPayloadText.size()),
                             std::span<const std::uint8_t>{
                                 reinterpret_cast<const std::uint8_t*>(kGoldenPayloadText.data()),
                                 kGoldenPayloadText.size()}));
    }

    SECTION("Should be padded to a whole flash word with the erased-flash byte") {
      const auto parsed = ParseImageHeader(kGoldenContainer);
      const auto payload = ContainerPayload(parsed.header, kGoldenContainer);

      REQUIRE(payload.has_value());
      REQUIRE(std::ranges::all_of(payload->subspan(kGoldenPayloadText.size()),
                                  [](std::uint8_t byte) { return byte == 0xFF; }));
    }
  }

  SECTION("When the whole container is evaluated against the ADC board application slot") {
    SECTION("Should be declared installable, checksum included") {
      const auto parsed = ParseImageHeader(kGoldenContainer);
      const auto payload = ContainerPayload(parsed.header, kGoldenContainer);

      REQUIRE(payload.has_value());
      REQUIRE(EvaluateImageInstallability(parsed.header, *payload, MakeAdcBoardConstraints()) ==
              ImageInstallability::kInstallable);
    }
  }
}

#endif
