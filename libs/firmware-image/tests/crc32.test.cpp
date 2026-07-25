#if defined(UNIT_TESTS)

#include "firmware-image/crc32.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <span>

namespace {

using midismith::firmware_image::ComputeCrc32;
using midismith::firmware_image::Crc32Accumulator;

}  // namespace

TEST_CASE("The ComputeCrc32 function") {
  SECTION("When given the standard CRC-32 check vector") {
    SECTION("Should produce the CRC-32/ISO-HDLC check value that zlib.crc32 also produces") {
      constexpr std::array<std::uint8_t, 9> check_vector = {'1', '2', '3', '4', '5',
                                                            '6', '7', '8', '9'};

      const std::uint32_t checksum = ComputeCrc32(check_vector);

      REQUIRE(checksum == 0xCBF43926u);
    }
  }

  SECTION("When given no data") {
    SECTION("Should produce zero") {
      const std::uint32_t checksum = ComputeCrc32(std::span<const std::uint8_t>{});

      REQUIRE(checksum == 0u);
    }
  }

  SECTION("When a single bit of the data changes") {
    SECTION("Should produce a different checksum") {
      const std::array<std::uint8_t, 4> original = {0x00, 0x01, 0x02, 0x03};
      const std::array<std::uint8_t, 4> mutated = {0x00, 0x01, 0x02, 0x02};

      REQUIRE(ComputeCrc32(original) != ComputeCrc32(mutated));
    }
  }

  SECTION("When the data differs only by a leading zero byte") {
    SECTION("Should produce a different checksum so length is covered") {
      const std::array<std::uint8_t, 2> short_data = {0xAA, 0xBB};
      const std::array<std::uint8_t, 3> padded_data = {0x00, 0xAA, 0xBB};

      REQUIRE(ComputeCrc32(short_data) != ComputeCrc32(padded_data));
    }
  }
}

TEST_CASE("The Crc32Accumulator class") {
  SECTION("The Update() method") {
    SECTION("When the same data is fed in several chunks") {
      SECTION("Should match the checksum of the whole buffer fed at once") {
        const std::array<std::uint8_t, 6> whole = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
        const std::span<const std::uint8_t> whole_span{whole};

        Crc32Accumulator accumulator;
        accumulator.Update(whole_span.first(1));
        accumulator.Update(whole_span.subspan(1, 4));
        accumulator.Update(whole_span.last(1));

        REQUIRE(accumulator.value() == ComputeCrc32(whole));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after data was accumulated") {
      SECTION("Should return the accumulator to its empty-input checksum") {
        const std::array<std::uint8_t, 3> data = {0x01, 0x02, 0x03};

        Crc32Accumulator accumulator;
        accumulator.Update(data);
        accumulator.Reset();

        REQUIRE(accumulator.value() == ComputeCrc32(std::span<const std::uint8_t>{}));
      }
    }
  }
}

#endif
