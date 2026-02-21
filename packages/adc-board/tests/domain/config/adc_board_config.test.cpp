#if defined(UNIT_TESTS)

#include "domain/config/adc_board_config.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "domain/config/config_validator.hpp"

namespace {

domain::config::AdcBoardConfig CreateBoardConfig(std::uint8_t board_id,
                                                 std::uint16_t version) noexcept {
  auto config = domain::config::CreateDefaultAdcBoardConfig();
  config.version = version;
  config.data.can_board_id = board_id;
  domain::config::ConfigValidator<domain::config::AdcBoardConfig>::StampCrc(config);
  return config;
}

}  // namespace

TEST_CASE("The CreateDefaultAdcBoardConfig function") {
  auto config = domain::config::CreateDefaultAdcBoardConfig();

  REQUIRE(config.magic_number == domain::config::kAdcMagic);
  REQUIRE(config.version == domain::config::kAdcVersion);
  REQUIRE(config.data.can_board_id == domain::config::kDefaultBoardId);
  REQUIRE(domain::config::ConfigValidator<domain::config::AdcBoardConfig>::Validate(config) ==
          domain::config::ConfigStatus::kValid);
}

TEST_CASE("The IsValidBoardId function") {
  REQUIRE(domain::config::IsValidBoardId(1));
  REQUIRE(domain::config::IsValidBoardId(4));
  REQUIRE(domain::config::IsValidBoardId(8));

  REQUIRE_FALSE(domain::config::IsValidBoardId(0));
  REQUIRE_FALSE(domain::config::IsValidBoardId(9));
  REQUIRE_FALSE(domain::config::IsValidBoardId(255));
}

TEST_CASE("The MigrateAdcBoardConfig function") {
  SECTION("When old board ID is valid") {
    auto old_config = CreateBoardConfig(5, 1);

    auto migrated = domain::config::MigrateAdcBoardConfig(old_config, old_config.version);

    REQUIRE(migrated.data.can_board_id == 5);
    REQUIRE(migrated.version == domain::config::kAdcVersion);
    REQUIRE(migrated.magic_number == domain::config::kAdcMagic);
    REQUIRE(domain::config::ConfigValidator<domain::config::AdcBoardConfig>::Validate(migrated) ==
            domain::config::ConfigStatus::kValid);
  }

  SECTION("When old board ID is invalid") {
    auto old_config = CreateBoardConfig(99, 1);

    auto migrated = domain::config::MigrateAdcBoardConfig(old_config, old_config.version);

    REQUIRE(migrated.data.can_board_id == domain::config::kDefaultBoardId);
    REQUIRE(domain::config::ConfigValidator<domain::config::AdcBoardConfig>::Validate(migrated) ==
            domain::config::ConfigStatus::kValid);
  }
}

#endif
