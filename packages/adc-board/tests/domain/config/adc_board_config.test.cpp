#if defined(UNIT_TESTS)

#include "domain/config/adc_board_config.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "domain/config/config_validator.hpp"

namespace {

midismith::adc_board::domain::config::AdcBoardConfig CreateBoardConfig(
    std::uint8_t board_id, std::uint16_t version) noexcept {
  auto config = midismith::adc_board::domain::config::CreateDefaultAdcBoardConfig();
  config.version = version;
  config.data.can_board_id = board_id;
  midismith::adc_board::domain::config::ConfigValidator<
      midismith::adc_board::domain::config::AdcBoardConfig>::StampCrc(config);
  return config;
}

}  // namespace

TEST_CASE("The CreateDefaultAdcBoardConfig function") {
  auto config = midismith::adc_board::domain::config::CreateDefaultAdcBoardConfig();

  REQUIRE(config.magic_number == midismith::adc_board::domain::config::kAdcMagic);
  REQUIRE(config.version == midismith::adc_board::domain::config::kAdcVersion);
  REQUIRE(config.data.can_board_id == midismith::adc_board::domain::config::kDefaultBoardId);
  REQUIRE(midismith::adc_board::domain::config::ConfigValidator<
              midismith::adc_board::domain::config::AdcBoardConfig>::Validate(config) ==
          midismith::adc_board::domain::config::ConfigStatus::kValid);
}

TEST_CASE("The IsValidBoardId function") {
  REQUIRE(midismith::adc_board::domain::config::IsValidBoardId(1));
  REQUIRE(midismith::adc_board::domain::config::IsValidBoardId(4));
  REQUIRE(midismith::adc_board::domain::config::IsValidBoardId(8));

  REQUIRE_FALSE(midismith::adc_board::domain::config::IsValidBoardId(0));
  REQUIRE_FALSE(midismith::adc_board::domain::config::IsValidBoardId(9));
  REQUIRE_FALSE(midismith::adc_board::domain::config::IsValidBoardId(255));
}

TEST_CASE("The MigrateAdcBoardConfig function") {
  SECTION("When old board ID is valid") {
    auto old_config = CreateBoardConfig(5, 1);

    auto migrated =
        midismith::adc_board::domain::config::MigrateAdcBoardConfig(old_config, old_config.version);

    REQUIRE(migrated.data.can_board_id == 5);
    REQUIRE(migrated.version == midismith::adc_board::domain::config::kAdcVersion);
    REQUIRE(migrated.magic_number == midismith::adc_board::domain::config::kAdcMagic);
    REQUIRE(midismith::adc_board::domain::config::ConfigValidator<
                midismith::adc_board::domain::config::AdcBoardConfig>::Validate(migrated) ==
            midismith::adc_board::domain::config::ConfigStatus::kValid);
  }

  SECTION("When old board ID is invalid") {
    auto old_config = CreateBoardConfig(99, 1);

    auto migrated =
        midismith::adc_board::domain::config::MigrateAdcBoardConfig(old_config, old_config.version);

    REQUIRE(migrated.data.can_board_id == midismith::adc_board::domain::config::kDefaultBoardId);
    REQUIRE(midismith::adc_board::domain::config::ConfigValidator<
                midismith::adc_board::domain::config::AdcBoardConfig>::Validate(migrated) ==
            midismith::adc_board::domain::config::ConfigStatus::kValid);
  }
}

#endif
