#if defined(UNIT_TESTS)

#include "firmware-image/image_header.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <string_view>

#include "firmware-image/product_id.hpp"

namespace {

using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::ImageHeaderStatus;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::kSupportedFormatVersion;
using midismith::firmware_image::ParseImageHeader;
using midismith::firmware_image::ProductId;

using HeaderBuffer = std::array<std::uint8_t, kImageHeaderSizeBytes>;

template <std::size_t kCapacity>
std::array<char, kCapacity> MakeTextField(std::string_view text) {
  std::array<char, kCapacity> field{};
  const std::size_t copied_length = std::min(text.size(), kCapacity - 1);
  std::copy_n(text.begin(), copied_length, field.begin());
  return field;
}

ImageHeader MakeAdcBoardHeader() {
  ImageHeader header;
  header.product_id = ProductId::kAdcBoard;
  header.payload_size_bytes = 125136;
  header.payload_crc32 = 0xDEADBEEF;
  header.load_address = 0x08100000;
  header.min_compatible_protocol_version = 3;
  header.version_string = MakeTextField<32>("v1.3.0-4-gab12cd34ef");
  header.build_date = MakeTextField<32>("2026-07-25T10:31:07+02:00");
  return header;
}

HeaderBuffer SerializeOrFail(const ImageHeader& header) {
  HeaderBuffer buffer{};
  const auto written_length = header.Serialize(buffer);
  REQUIRE(written_length.has_value());
  REQUIRE(*written_length == kImageHeaderSizeBytes);
  return buffer;
}

}  // namespace

TEST_CASE("The ImageHeader struct") {
  SECTION("The Serialize() method") {
    SECTION("When the destination buffer is smaller than a header") {
      SECTION("Should refuse to write anything") {
        std::array<std::uint8_t, kImageHeaderSizeBytes - 1> undersized_buffer{};

        const auto written_length = MakeAdcBoardHeader().Serialize(undersized_buffer);

        REQUIRE_FALSE(written_length.has_value());
      }
    }

    SECTION("When the destination buffer is large enough") {
      SECTION("Should open the image with the MSFW magic") {
        const HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());

        REQUIRE(buffer[0] == 'M');
        REQUIRE(buffer[1] == 'S');
        REQUIRE(buffer[2] == 'F');
        REQUIRE(buffer[3] == 'W');
      }

      SECTION("Should produce identical bytes for identical headers so builds stay reproducible") {
        const HeaderBuffer first = SerializeOrFail(MakeAdcBoardHeader());
        const HeaderBuffer second = SerializeOrFail(MakeAdcBoardHeader());

        REQUIRE(first == second);
      }
    }
  }
}

TEST_CASE("The ParseImageHeader function") {
  SECTION("When given a freshly serialized header") {
    SECTION("Should accept it") {
      const HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kValid);
    }

    SECTION("Should recover every field unchanged") {
      const ImageHeader original = MakeAdcBoardHeader();
      const HeaderBuffer buffer = SerializeOrFail(original);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.header == original);
    }
  }

  SECTION("When the buffer is shorter than a header") {
    SECTION("Should reject it rather than read past the end") {
      const HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      const std::span<const std::uint8_t> truncated =
          std::span<const std::uint8_t>{buffer}.first(kImageHeaderSizeBytes - 1);

      REQUIRE(ParseImageHeader(truncated).status == ImageHeaderStatus::kBufferTooSmall);
    }
  }

  SECTION("When the magic does not match") {
    SECTION("Should reject it so an arbitrary file is never mistaken for a firmware image") {
      HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      buffer[0] = 'X';

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kMagicMismatch);
    }
  }

  SECTION("When any header byte was corrupted in transit") {
    SECTION("Should reject it on the header checksum") {
      HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      buffer[0x08] ^= 0x01;

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kHeaderChecksumMismatch);
    }
  }

  SECTION("When the format version is not the one this firmware understands") {
    SECTION("Should reject it instead of guessing the field layout") {
      ImageHeader future_header = MakeAdcBoardHeader();
      future_header.format_version = kSupportedFormatVersion + 1;
      const HeaderBuffer buffer = SerializeOrFail(future_header);

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kUnsupportedFormatVersion);
    }
  }

  SECTION("When the product id is not a product this firmware knows") {
    SECTION("Should report it as unknown so it can never match a target") {
      ImageHeader alien_header = MakeAdcBoardHeader();
      alien_header.product_id = static_cast<ProductId>(0x4242);
      const HeaderBuffer buffer = SerializeOrFail(alien_header);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.header.product_id == ProductId::kUnknown);
    }
  }

  SECTION("When a text field fills its whole capacity") {
    SECTION("Should drop the last character so the field stays readable as a C string") {
      ImageHeader header = MakeAdcBoardHeader();
      std::fill(header.version_string.begin(), header.version_string.end(), 'v');
      const HeaderBuffer buffer = SerializeOrFail(header);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.header.version_string[30] == 'v');
      REQUIRE(parsed.header.version_string[31] == '\0');
    }
  }
}

#endif
