#if defined(UNIT_TESTS)

#include "domain/hash/crc32.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>

TEST_CASE("The ComputeCrc32 function") {
  using midismith::adc_board::domain::hash::ComputeCrc32;

  SECTION("The standard check value") {
    SECTION("Should return 0xCBF43926 for the string '123456789'") {
      const char* input = "123456789";
      auto result = ComputeCrc32(reinterpret_cast<const std::uint8_t*>(input), std::strlen(input));
      REQUIRE(result == 0xCBF43926u);
    }
  }

  SECTION("When data is empty") {
    SECTION("Should return 0x00000000") {
      auto result = ComputeCrc32(nullptr, 0);
      REQUIRE(result == 0x00000000u);
    }
  }

  SECTION("When data is a single zero byte") {
    SECTION("Should return 0xD202EF8D") {
      std::uint8_t byte = 0x00;
      auto result = ComputeCrc32(&byte, 1);
      REQUIRE(result == 0xD202EF8Du);
    }
  }

  SECTION("When called with the same data twice") {
    SECTION("Should return the same result") {
      const char* input = "deterministic";
      auto first = ComputeCrc32(reinterpret_cast<const std::uint8_t*>(input), std::strlen(input));
      auto second = ComputeCrc32(reinterpret_cast<const std::uint8_t*>(input), std::strlen(input));
      REQUIRE(first == second);
    }
  }
}

#endif
