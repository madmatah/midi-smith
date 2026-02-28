#if defined(UNIT_TESTS)

#include <array>
#include <cstdint>

#include "byte-codec/little_endian.hpp"
#include <catch2/catch_test_macros.hpp>

using midismith::byte_codec::ReadLittleEndian;
using midismith::byte_codec::WriteLittleEndian;

TEST_CASE("The WriteLittleEndian function") {
  SECTION("When writing a uint8_t value") {
    SECTION("Should write the byte at the given offset") {
      std::array<std::uint8_t, 4> buffer{};
      WriteLittleEndian<std::uint8_t>(buffer, 2, 0xAB);
      REQUIRE(buffer[2] == 0xAB);
    }
  }

  SECTION("When writing a uint16_t value") {
    SECTION("Should write the LSB at offset and MSB at offset+1") {
      std::array<std::uint8_t, 4> buffer{};
      WriteLittleEndian<std::uint16_t>(buffer, 1, 0x1234);
      REQUIRE(buffer[1] == 0x34);
      REQUIRE(buffer[2] == 0x12);
    }
  }

  SECTION("When writing a uint32_t value") {
    SECTION("Should write the four bytes in LSB-first order") {
      std::array<std::uint8_t, 4> buffer{};
      WriteLittleEndian<std::uint32_t>(buffer, 0, 0x12345678);
      REQUIRE(buffer[0] == 0x78);
      REQUIRE(buffer[1] == 0x56);
      REQUIRE(buffer[2] == 0x34);
      REQUIRE(buffer[3] == 0x12);
    }
  }
}

TEST_CASE("The ReadLittleEndian function") {
  SECTION("When reading a uint8_t value") {
    SECTION("Should return the byte at the given offset") {
      const std::array<std::uint8_t, 4> buffer{0x00, 0x00, 0xAB, 0x00};
      REQUIRE(ReadLittleEndian<std::uint8_t>(buffer, 2) == 0xAB);
    }
  }

  SECTION("When reading a uint16_t value") {
    SECTION("Should reconstruct the value from LSB-first bytes") {
      const std::array<std::uint8_t, 4> buffer{0x00, 0x34, 0x12, 0x00};
      REQUIRE(ReadLittleEndian<std::uint16_t>(buffer, 1) == 0x1234);
    }
  }

  SECTION("When reading a uint32_t value") {
    SECTION("Should reconstruct the value from four LSB-first bytes") {
      const std::array<std::uint8_t, 4> buffer{0x78, 0x56, 0x34, 0x12};
      REQUIRE(ReadLittleEndian<std::uint32_t>(buffer, 0) == 0x12345678);
    }
  }
}

TEST_CASE("The WriteLittleEndian and ReadLittleEndian functions used together") {
  SECTION("When a uint8_t value is written and then read") {
    SECTION("Should return the original value") {
      std::array<std::uint8_t, 1> buffer{};
      WriteLittleEndian<std::uint8_t>(buffer, 0, 0xFF);
      REQUIRE(ReadLittleEndian<std::uint8_t>(buffer, 0) == 0xFF);
    }
  }

  SECTION("When a uint16_t value is written and then read") {
    SECTION("Should return the original value") {
      std::array<std::uint8_t, 2> buffer{};
      WriteLittleEndian<std::uint16_t>(buffer, 0, 0xBEEF);
      REQUIRE(ReadLittleEndian<std::uint16_t>(buffer, 0) == 0xBEEF);
    }
  }

  SECTION("When a uint32_t value is written and then read") {
    SECTION("Should return the original value") {
      std::array<std::uint8_t, 4> buffer{};
      WriteLittleEndian<std::uint32_t>(buffer, 0, 0xDEADBEEF);
      REQUIRE(ReadLittleEndian<std::uint32_t>(buffer, 0) == 0xDEADBEEF);
    }
  }
}

#endif
