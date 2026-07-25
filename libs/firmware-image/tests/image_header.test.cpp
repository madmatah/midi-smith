#if defined(UNIT_TESTS)

#include "firmware-image/image_header.hpp"

#include <algorithm>
#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>
#include <vector>

#include "firmware-image/product_id.hpp"
#include "image_fixtures.hpp"

namespace {

using midismith::firmware_image::ContainerPayload;
using midismith::firmware_image::ImageHeader;
using midismith::firmware_image::ImageHeaderStatus;
using midismith::firmware_image::kBuildDateCapacity;
using midismith::firmware_image::kImageHeaderSizeBytes;
using midismith::firmware_image::kSupportedFormatVersion;
using midismith::firmware_image::kVersionStringCapacity;
using midismith::firmware_image::ParseImageHeader;
using midismith::firmware_image::ProductId;
using midismith::firmware_image::test::MakeAdcBoardHeader;
using midismith::firmware_image::test::MakePayload;

using HeaderBuffer = std::array<std::uint8_t, kImageHeaderSizeBytes>;

constexpr std::size_t kPayloadSizeFieldOffset = 0x08;
constexpr std::size_t kLastMagicByteOffset = 3;
constexpr std::uint16_t kUnassignedProductRawValue = 0x4242;
constexpr std::uint8_t kUntouchedFillByte = 0xA5;

HeaderBuffer SerializeOrFail(const ImageHeader& header) {
  HeaderBuffer buffer{};
  const auto written_length_bytes = header.Serialize(buffer);
  REQUIRE(written_length_bytes.has_value());
  REQUIRE(*written_length_bytes == kImageHeaderSizeBytes);
  return buffer;
}

}  // namespace

TEST_CASE("The ImageHeader struct") {
  SECTION("The Serialize() method") {
    SECTION("When the destination buffer is smaller than a header") {
      SECTION("Should refuse to write anything, leaving the buffer as it found it") {
        std::array<std::uint8_t, kImageHeaderSizeBytes - 1> undersized_buffer{};
        undersized_buffer.fill(kUntouchedFillByte);

        const auto written_length_bytes = MakeAdcBoardHeader().Serialize(undersized_buffer);

        REQUIRE_FALSE(written_length_bytes.has_value());
        REQUIRE(std::ranges::all_of(undersized_buffer,
                                    [](std::uint8_t byte) { return byte == kUntouchedFillByte; }));
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

      SECTION("Should write no more than a header even when the buffer is longer") {
        std::array<std::uint8_t, kImageHeaderSizeBytes * 2> oversized_buffer{};
        oversized_buffer.fill(kUntouchedFillByte);

        const auto written_length_bytes = MakeAdcBoardHeader().Serialize(oversized_buffer);

        REQUIRE(written_length_bytes.has_value());
        REQUIRE(std::ranges::all_of(std::span{oversized_buffer}.subspan(kImageHeaderSizeBytes),
                                    [](std::uint8_t byte) { return byte == kUntouchedFillByte; }));
      }
    }
  }
}

TEST_CASE("The ParseImageHeader function") {
  SECTION("When given a freshly serialized header") {
    SECTION("Should accept it") {
      const HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.is_valid());
    }

    SECTION("Should recover every field unchanged") {
      const ImageHeader original = MakeAdcBoardHeader();
      const HeaderBuffer buffer = SerializeOrFail(original);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.header == original);
    }
  }

  SECTION("When given a whole container rather than a bare header") {
    SECTION("Should read only the leading header bytes and still accept it") {
      const std::vector<std::uint8_t> payload = MakePayload(2);
      const HeaderBuffer header_bytes = SerializeOrFail(MakeAdcBoardHeader());
      std::vector<std::uint8_t> container(header_bytes.begin(), header_bytes.end());
      container.insert(container.end(), payload.begin(), payload.end());

      const auto parsed = ParseImageHeader(container);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.header == MakeAdcBoardHeader());
    }
  }

  SECTION("When the buffer is shorter than a header") {
    SECTION("Should reject it rather than read past the end") {
      const HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      const std::span<const std::uint8_t> truncated =
          std::span<const std::uint8_t>{buffer}.first(kImageHeaderSizeBytes - 1);

      const auto parsed = ParseImageHeader(truncated);

      REQUIRE(parsed.status == ImageHeaderStatus::kBufferTooSmall);
      REQUIRE_FALSE(parsed.is_valid());
    }
  }

  SECTION("When the buffer is empty") {
    SECTION("Should reject it without touching the pointer") {
      REQUIRE(ParseImageHeader(std::span<const std::uint8_t>{}).status ==
              ImageHeaderStatus::kBufferTooSmall);
    }
  }

  SECTION("When the first byte of the magic differs") {
    SECTION("Should reject it so an arbitrary file is never mistaken for a firmware image") {
      HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      buffer[0] = 'X';

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kMagicMismatch);
    }
  }

  SECTION("When only the last byte of the magic differs") {
    SECTION("Should still reject it, because the whole word identifies the format") {
      HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      buffer[kLastMagicByteOffset] = 'X';

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kMagicMismatch);
    }
  }

  SECTION("When any header byte was corrupted in transit") {
    SECTION("Should reject it on the header checksum") {
      HeaderBuffer buffer = SerializeOrFail(MakeAdcBoardHeader());
      buffer[kPayloadSizeFieldOffset] ^= 0x01;

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kHeaderChecksumMismatch);
    }
  }

  SECTION("When the format version is newer than the one this firmware understands") {
    SECTION("Should reject it instead of guessing the field layout") {
      ImageHeader future_header = MakeAdcBoardHeader();
      future_header.format_version = kSupportedFormatVersion + 1;
      const HeaderBuffer buffer = SerializeOrFail(future_header);

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kUnsupportedFormatVersion);
    }
  }

  SECTION("When the format version predates the one this firmware understands") {
    SECTION("Should reject it too, because an older layout is as unreadable as a newer one") {
      ImageHeader ancient_header = MakeAdcBoardHeader();
      ancient_header.format_version = 0;
      const HeaderBuffer buffer = SerializeOrFail(ancient_header);

      REQUIRE(ParseImageHeader(buffer).status == ImageHeaderStatus::kUnsupportedFormatVersion);
    }
  }

  SECTION("When the product id is not a product this firmware knows") {
    SECTION("Should report it as unknown so it can never match a target") {
      ImageHeader alien_header = MakeAdcBoardHeader();
      alien_header.product_id = static_cast<ProductId>(kUnassignedProductRawValue);
      const HeaderBuffer buffer = SerializeOrFail(alien_header);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.header.product_id == ProductId::kUnknown);
    }
  }

  SECTION("When a text field fills its whole capacity") {
    SECTION("Should drop the last character so the field stays readable as a C string") {
      ImageHeader header = MakeAdcBoardHeader();
      header.version_string.fill('v');
      header.build_date.fill('d');
      const HeaderBuffer buffer = SerializeOrFail(header);

      const auto parsed = ParseImageHeader(buffer);

      REQUIRE(parsed.status == ImageHeaderStatus::kValid);
      REQUIRE(parsed.header.version_string[kVersionStringCapacity - 2] == 'v');
      REQUIRE(parsed.header.version_string[kVersionStringCapacity - 1] == '\0');
      REQUIRE(parsed.header.build_date[kBuildDateCapacity - 2] == 'd');
      REQUIRE(parsed.header.build_date[kBuildDateCapacity - 1] == '\0');
    }
  }
}

TEST_CASE("The ContainerPayload function") {
  SECTION("When the container carries exactly the payload the header announces") {
    SECTION("Should return that payload") {
      const std::vector<std::uint8_t> payload = MakePayload(2);
      ImageHeader header = MakeAdcBoardHeader();
      header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
      std::vector<std::uint8_t> container(kImageHeaderSizeBytes, 0);
      container.insert(container.end(), payload.begin(), payload.end());

      const auto extracted = ContainerPayload(header, container);

      REQUIRE(extracted.has_value());
      REQUIRE(std::ranges::equal(*extracted, payload));
    }
  }

  SECTION("When the container carries more bytes than the header announces") {
    SECTION("Should return only the announced payload, ignoring the trailing bytes") {
      const std::vector<std::uint8_t> payload = MakePayload(2);
      ImageHeader header = MakeAdcBoardHeader();
      header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
      std::vector<std::uint8_t> container(kImageHeaderSizeBytes, 0);
      container.insert(container.end(), payload.begin(), payload.end());
      container.push_back(0x5A);

      const auto extracted = ContainerPayload(header, container);

      REQUIRE(extracted.has_value());
      REQUIRE(extracted->size() == payload.size());
    }
  }

  SECTION("When the container was truncated before the payload was complete") {
    SECTION("Should refuse rather than hand back a span past the end of the buffer") {
      const std::vector<std::uint8_t> payload = MakePayload(2);
      ImageHeader header = MakeAdcBoardHeader();
      header.payload_size_bytes = static_cast<std::uint32_t>(payload.size());
      std::vector<std::uint8_t> container(kImageHeaderSizeBytes, 0);
      container.insert(container.end(), payload.begin(), payload.end() - 1);

      REQUIRE_FALSE(ContainerPayload(header, container).has_value());
    }
  }

  SECTION("When a power cut left only the header on the card") {
    SECTION("Should refuse, because a header alone announces a payload that is not there") {
      ImageHeader header = MakeAdcBoardHeader();
      const std::vector<std::uint8_t> container(kImageHeaderSizeBytes, 0);

      REQUIRE_FALSE(ContainerPayload(header, container).has_value());
    }
  }

  SECTION("When the buffer is shorter than a header") {
    SECTION("Should refuse rather than subtract past zero") {
      const std::vector<std::uint8_t> container(kImageHeaderSizeBytes - 1, 0);

      REQUIRE_FALSE(ContainerPayload(MakeAdcBoardHeader(), container).has_value());
    }
  }
}

#endif
