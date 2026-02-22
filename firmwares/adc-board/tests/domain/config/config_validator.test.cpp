#if defined(UNIT_TESTS)

#include "domain/config/config_validator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "domain/config/storable_config.hpp"

namespace {

struct TestData {
  std::uint8_t value;
};

using TestConfig = midismith::adc_board::domain::config::StorableConfig<TestData, 0x54455354u, 3>;

TestConfig CreateConfig(std::uint8_t value = 7, std::uint16_t version = TestConfig::kVersion) {
  TestConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.magic_number = TestConfig::kMagicNumber;
  config.version = version;
  config.data.value = value;
  midismith::adc_board::domain::config::ConfigValidator<TestConfig>::StampCrc(config);
  return config;
}

TestConfig CreateVirginFlashConfig() {
  TestConfig config{};
  std::memset(&config, 0xFF, sizeof(config));
  return config;
}

}  // namespace

TEST_CASE("The ValidateConfig function") {
  using midismith::adc_board::domain::config::ConfigStatus;

  SECTION("When flash is virgin (all 0xFF)") {
    SECTION("Should return kVirginFlash") {
      auto config = CreateVirginFlashConfig();
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kVirginFlash);
    }
  }

  SECTION("When config is valid with current version") {
    SECTION("Should return kValid") {
      auto config = CreateConfig();
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kValid);
    }
  }

  SECTION("When magic number is wrong") {
    SECTION("Should return kInvalidMagic") {
      auto config = CreateConfig();
      config.magic_number = 0xDEADBEEFu;
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kInvalidMagic);
    }
  }

  SECTION("When CRC is corrupted") {
    SECTION("Should return kInvalidCrc") {
      auto config = CreateConfig();
      config.crc32 ^= 0x01u;
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kInvalidCrc);
    }
  }

  SECTION("When data is corrupted but CRC is unchanged") {
    SECTION("Should return kInvalidCrc") {
      auto config = CreateConfig();
      config.data.value = 99;
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kInvalidCrc);
    }
  }

  SECTION("When version is older than current") {
    SECTION("Should return kOlderVersion") {
      auto config = CreateConfig(3, TestConfig::kVersion - 1);
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kOlderVersion);
    }
  }

  SECTION("When version is newer than current") {
    SECTION("Should return kNewerVersion") {
      auto config = CreateConfig(3, TestConfig::kVersion + 1);
      REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
              ConfigStatus::kNewerVersion);
    }
  }
}

TEST_CASE("The StampCrc function") {
  auto config = CreateConfig();
  config.crc32 = 0;

  midismith::adc_board::domain::config::ConfigValidator<TestConfig>::StampCrc(config);

  REQUIRE(midismith::adc_board::domain::config::ConfigValidator<TestConfig>::Validate(config) ==
          midismith::adc_board::domain::config::ConfigStatus::kValid);
}

#endif
