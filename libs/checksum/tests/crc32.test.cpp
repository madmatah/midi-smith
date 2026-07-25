#if defined(UNIT_TESTS)

#include "checksum/crc32.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <span>

namespace {

using midismith::checksum::ComputeCrc32;
using midismith::checksum::Crc32Accumulator;

constexpr std::uint32_t kCheckVectorChecksum = 0xCBF43926u;
constexpr std::uint32_t kEmptyInputChecksum = 0x00000000u;
constexpr std::uint32_t kSingleZeroByteChecksum = 0xD202EF8Du;

std::span<const std::uint8_t> AsBytes(const char* text) {
  return {reinterpret_cast<const std::uint8_t*>(text), std::strlen(text)};
}

}  // namespace

TEST_CASE("The ComputeCrc32 function") {
  SECTION("When given the standard CRC-32 check vector") {
    SECTION("Should produce the CRC-32/ISO-HDLC check value that zlib.crc32 also produces") {
      REQUIRE(ComputeCrc32(AsBytes("123456789")) == kCheckVectorChecksum);
    }
  }

  SECTION("When given no data") {
    SECTION("Should produce zero") {
      REQUIRE(ComputeCrc32(std::span<const std::uint8_t>{}) == kEmptyInputChecksum);
    }
  }

  SECTION("When given a null pointer and a zero length") {
    SECTION("Should produce the empty-input checksum rather than dereference it") {
      REQUIRE(ComputeCrc32(nullptr, 0) == kEmptyInputChecksum);
    }
  }

  SECTION("When given a single zero byte") {
    SECTION("Should distinguish it from no data at all") {
      constexpr std::array<std::uint8_t, 1> single_zero_byte = {0x00};

      REQUIRE(ComputeCrc32(single_zero_byte) == kSingleZeroByteChecksum);
      REQUIRE(ComputeCrc32(single_zero_byte) != kEmptyInputChecksum);
    }
  }

  SECTION("When a single bit of the data changes") {
    SECTION("Should produce a different checksum") {
      constexpr std::array<std::uint8_t, 4> original = {0x00, 0x01, 0x02, 0x03};
      constexpr std::array<std::uint8_t, 4> mutated = {0x00, 0x01, 0x02, 0x02};

      REQUIRE(ComputeCrc32(original) != ComputeCrc32(mutated));
    }
  }

  SECTION("When the data differs only by a leading zero byte") {
    SECTION("Should produce a different checksum so length is covered") {
      constexpr std::array<std::uint8_t, 2> short_data = {0xAA, 0xBB};
      constexpr std::array<std::uint8_t, 3> padded_data = {0x00, 0xAA, 0xBB};

      REQUIRE(ComputeCrc32(short_data) != ComputeCrc32(padded_data));
    }
  }

  SECTION("When called twice with the same data") {
    SECTION("Should produce the same checksum") {
      REQUIRE(ComputeCrc32(AsBytes("deterministic")) == ComputeCrc32(AsBytes("deterministic")));
    }
  }

  SECTION("When evaluated in a constant expression") {
    SECTION("Should be usable at compile time so callers can pin checksums in static_asserts") {
      constexpr std::array<std::uint8_t, 9> check_vector = {'1', '2', '3', '4', '5',
                                                            '6', '7', '8', '9'};
      static_assert(ComputeCrc32(check_vector) == kCheckVectorChecksum);

      SUCCEED();
    }
  }
}

TEST_CASE("The Crc32Accumulator class") {
  SECTION("The Update() method") {
    SECTION("When the same data is fed in several chunks") {
      SECTION("Should match the checksum of the whole buffer fed at once") {
        constexpr std::array<std::uint8_t, 6> whole = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
        const std::span<const std::uint8_t> whole_span{whole};

        Crc32Accumulator accumulator;
        accumulator.Update(whole_span.first(1));
        accumulator.Update(whole_span.subspan(1, 4));
        accumulator.Update(whole_span.last(1));

        REQUIRE(accumulator.value() == ComputeCrc32(whole));
      }
    }

    SECTION("When fed an empty chunk between two real ones") {
      SECTION("Should leave the checksum unchanged so a stalled transfer costs nothing") {
        constexpr std::array<std::uint8_t, 2> whole = {0x7E, 0x11};
        const std::span<const std::uint8_t> whole_span{whole};

        Crc32Accumulator accumulator;
        accumulator.Update(whole_span.first(1));
        accumulator.Update(std::span<const std::uint8_t>{});
        accumulator.Update(whole_span.last(1));

        REQUIRE(accumulator.value() == ComputeCrc32(whole));
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When called after data was accumulated") {
      SECTION("Should return the accumulator to its empty-input checksum") {
        constexpr std::array<std::uint8_t, 3> data = {0x01, 0x02, 0x03};

        Crc32Accumulator accumulator;
        accumulator.Update(data);
        accumulator.Reset();

        REQUIRE(accumulator.value() == kEmptyInputChecksum);
      }
    }
  }
}

#endif
