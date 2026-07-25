#if defined(UNIT_TESTS)

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <string_view>

#include "firmware-image/crc32.hpp"
#include "firmware-image/image_acceptance.hpp"
#include "firmware-image/image_header.hpp"
#include "firmware-image/product_id.hpp"

namespace {

using midismith::firmware_image::ComputeCrc32;
using midismith::firmware_image::EvaluateImageAcceptance;
using midismith::firmware_image::ImageAcceptance;
using midismith::firmware_image::ImageHeaderStatus;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::ParseImageHeader;
using midismith::firmware_image::ProductId;
using midismith::firmware_image::TargetConstraints;

constexpr std::string_view kGoldenPayload = "midi-smith firmware payload golden vector";

constexpr std::array<std::uint8_t, kImageHeaderSizeBytes> kGoldenHeaderBytes = {
    0x4D, 0x53, 0x46, 0x57, 0x01, 0x00, 0x02, 0x00, 0x29, 0x00, 0x00, 0x00, 0xFC, 0xCA, 0xE5, 0x8F,
    0x00, 0x00, 0x10, 0x08, 0x03, 0x00, 0x00, 0x00, 0x76, 0x31, 0x2E, 0x33, 0x2E, 0x30, 0x2D, 0x34,
    0x2D, 0x67, 0x61, 0x62, 0x31, 0x32, 0x63, 0x64, 0x33, 0x34, 0x65, 0x66, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x32, 0x30, 0x32, 0x36, 0x2D, 0x30, 0x37, 0x2D,
    0x32, 0x35, 0x54, 0x31, 0x30, 0x3A, 0x33, 0x31, 0x3A, 0x30, 0x37, 0x2B, 0x30, 0x32, 0x3A, 0x30,
    0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x5D, 0xBB, 0x95};

std::span<const std::uint8_t> GoldenPayloadBytes() {
  return {reinterpret_cast<const std::uint8_t*>(kGoldenPayload.data()), kGoldenPayload.size()};
}

}  // namespace

TEST_CASE("The .msfw container produced by tools/firmware_packager.py") {
  SECTION("When parsed by the firmware that will install it") {
    SECTION("Should be accepted, pinning the layout both implementations write") {
      const auto parsed = ParseImageHeader(kGoldenHeaderBytes);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
    }

    SECTION("Should carry the product, address and protocol the tool was given") {
      const auto parsed = ParseImageHeader(kGoldenHeaderBytes);

      REQUIRE(parsed.header.product_id == ProductId::kAdcBoard);
      REQUIRE(parsed.header.load_address == 0x08100000u);
      REQUIRE(parsed.header.min_compatible_protocol_version == 3u);
    }

    SECTION("Should carry the git version and commit date the build passed in") {
      const auto parsed = ParseImageHeader(kGoldenHeaderBytes);

      REQUIRE(std::string_view(parsed.header.version_string.data()) == "v1.3.0-4-gab12cd34ef");
      REQUIRE(std::string_view(parsed.header.build_date.data()) == "2026-07-25T10:31:07+02:00");
    }
  }

  SECTION("When its payload checksum is recomputed on the target") {
    SECTION("Should match the checksum zlib.crc32 wrote into the header") {
      const auto parsed = ParseImageHeader(kGoldenHeaderBytes);

      REQUIRE(parsed.header.payload_size_bytes == kGoldenPayload.size());
      REQUIRE(parsed.header.payload_crc32 == ComputeCrc32(GoldenPayloadBytes()));
    }
  }

  SECTION("When evaluated against the ADC board application slot") {
    SECTION("Should be accepted for installation") {
      const auto parsed = ParseImageHeader(kGoldenHeaderBytes);
      const TargetConstraints constraints{ProductId::kAdcBoard, 0x08100000u, 384u * 1024u, 3u};

      REQUIRE(EvaluateImageAcceptance(parsed.header, constraints) == ImageAcceptance::kAccepted);
    }
  }
}

#endif
