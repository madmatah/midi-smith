#if defined(UNIT_TESTS)

#include "domain/config/main_board_config.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstring>

#include "config/config_validator.hpp"

using midismith::main_board::domain::config::CreateDefaultMainBoardConfig;
using midismith::main_board::domain::config::IsValidKeymapEntry;
using midismith::main_board::domain::config::MainBoardConfig;

TEST_CASE("The CreateDefaultMainBoardConfig function") {
  auto config = CreateDefaultMainBoardConfig();

  SECTION("Should produce a valid config") {
    REQUIRE(midismith::config::ConfigValidator<MainBoardConfig>::Validate(config) ==
            midismith::config::ConfigStatus::kValid);
  }

  SECTION("Should have an empty keymap") {
    REQUIRE(config.data.key_count == 0);
    REQUIRE(config.data.start_note == 0);
    REQUIRE(config.data.entry_count == 0);
  }
}

TEST_CASE("The IsValidKeymapEntry function") {
  SECTION("Should accept a valid entry") {
    REQUIRE(IsValidKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 60}));
    REQUIRE(IsValidKeymapEntry({.board_id = 8, .sensor_id = 21, .midi_note = 127}));
  }

  SECTION("Should reject board_id 0") {
    REQUIRE_FALSE(IsValidKeymapEntry({.board_id = 0, .sensor_id = 0, .midi_note = 60}));
  }

  SECTION("Should reject board_id above max") {
    REQUIRE_FALSE(IsValidKeymapEntry({.board_id = 9, .sensor_id = 0, .midi_note = 60}));
  }

  SECTION("Should reject sensor_id at or above sensors per board") {
    REQUIRE_FALSE(IsValidKeymapEntry({.board_id = 1, .sensor_id = 22, .midi_note = 60}));
  }

  SECTION("Should reject midi_note above 127") {
    REQUIRE_FALSE(IsValidKeymapEntry({.board_id = 1, .sensor_id = 0, .midi_note = 128}));
  }
}

#endif
