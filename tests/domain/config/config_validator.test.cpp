#if defined(UNIT_TESTS)

#include "domain/config/config_validator.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "domain/hash/crc32.hpp"

namespace {

domain::config::UserConfig CreateValidConfig(
    std::uint8_t board_id = 1, std::uint16_t version = domain::config::kConfigVersion) {
  domain::config::UserConfig config{};
  std::memset(&config, 0, sizeof(config));
  config.magic_number = domain::config::kConfigMagicNumber;
  config.version = version;
  config.data.can_board_id = board_id;
  config.crc32 = domain::hash::ComputeCrc32(reinterpret_cast<const std::uint8_t*>(&config),
                                            offsetof(domain::config::UserConfig, crc32));
  return config;
}

domain::config::UserConfig CreateVirginFlashConfig() {
  domain::config::UserConfig config{};
  std::memset(&config, 0xFF, sizeof(config));
  return config;
}

}  // namespace

TEST_CASE("The ValidateConfig function") {
  using domain::config::ConfigStatus;
  using domain::config::ValidateConfig;

  SECTION("When flash is virgin (all 0xFF)") {
    SECTION("Should return kVirginFlash") {
      auto config = CreateVirginFlashConfig();
      REQUIRE(ValidateConfig(config) == ConfigStatus::kVirginFlash);
    }
  }

  SECTION("When config is valid with current version") {
    SECTION("Should return kValid") {
      auto config = CreateValidConfig();
      REQUIRE(ValidateConfig(config) == ConfigStatus::kValid);
    }
  }

  SECTION("When magic number is wrong") {
    SECTION("Should return kInvalidMagic") {
      auto config = CreateValidConfig();
      config.magic_number = 0xDEADBEEFu;
      REQUIRE(ValidateConfig(config) == ConfigStatus::kInvalidMagic);
    }
  }

  SECTION("When CRC is corrupted") {
    SECTION("Should return kInvalidCrc") {
      auto config = CreateValidConfig();
      config.crc32 ^= 0x01u;
      REQUIRE(ValidateConfig(config) == ConfigStatus::kInvalidCrc);
    }
  }

  SECTION("When data is corrupted but CRC is unchanged") {
    SECTION("Should return kInvalidCrc") {
      auto config = CreateValidConfig();
      config.data.can_board_id = 99;
      REQUIRE(ValidateConfig(config) == ConfigStatus::kInvalidCrc);
    }
  }

  SECTION("When version is older than current") {
    SECTION("Should return kOlderVersion") {
      auto config = CreateValidConfig(3, domain::config::kConfigVersion - 1);
      REQUIRE(ValidateConfig(config) == ConfigStatus::kOlderVersion);
    }
  }
}

TEST_CASE("The IsValidBoardId function") {
  using domain::config::IsValidBoardId;

  SECTION("When board ID is within range [1..8]") {
    SECTION("Should return true") {
      REQUIRE(IsValidBoardId(1));
      REQUIRE(IsValidBoardId(4));
      REQUIRE(IsValidBoardId(8));
    }
  }

  SECTION("When board ID is 0") {
    SECTION("Should return false") {
      REQUIRE_FALSE(IsValidBoardId(0));
    }
  }

  SECTION("When board ID is above maximum") {
    SECTION("Should return false") {
      REQUIRE_FALSE(IsValidBoardId(9));
      REQUIRE_FALSE(IsValidBoardId(255));
    }
  }
}

TEST_CASE("The MigrateConfig function") {
  using domain::config::ConfigStatus;
  using domain::config::MigrateConfig;
  using domain::config::ValidateConfig;

  SECTION("When migrating from version 1") {
    SECTION("Should preserve valid board ID") {
      auto old_config = CreateValidConfig(5, 1);
      auto migrated = MigrateConfig(old_config, 1);
      REQUIRE(migrated.data.can_board_id == 5);
      REQUIRE(migrated.version == domain::config::kConfigVersion);
      REQUIRE(migrated.magic_number == domain::config::kConfigMagicNumber);
      REQUIRE(ValidateConfig(migrated) == ConfigStatus::kValid);
    }

    SECTION("Should use default board ID when old value is invalid") {
      auto old_config = CreateValidConfig(99, 1);
      auto migrated = MigrateConfig(old_config, 1);
      REQUIRE(migrated.data.can_board_id == domain::config::kDefaultBoardId);
    }
  }
}

#endif
