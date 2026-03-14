#if defined(UNIT_TESTS)

#include "domain/keymap/keymap_lookup.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("The KeymapLookup class") {
  midismith::main_board::domain::config::MainBoardData data{};
  data.key_count = 88;
  data.start_note = 21;
  data.entry_count = 0;

  midismith::main_board::domain::config::KeymapLookup lookup(data);

  SECTION("When the keymap is empty") {
    REQUIRE_FALSE(lookup.FindMidiNote(1, 0).has_value());
  }

  SECTION("When the keymap contains entries") {
    data.entries[0] = {.board_id = 1, .sensor_id = 0, .midi_note = 21};
    data.entries[1] = {.board_id = 1, .sensor_id = 1, .midi_note = 21};
    data.entries[2] = {.board_id = 2, .sensor_id = 5, .midi_note = 60};
    data.entry_count = 3;

    SECTION("Should find a known mapping") {
      auto result = lookup.FindMidiNote(1, 0);
      REQUIRE(result.has_value());
      REQUIRE(*result == 21);
    }

    SECTION("Should return nullopt for unknown mapping") {
      REQUIRE_FALSE(lookup.FindMidiNote(3, 0).has_value());
    }

    SECTION("Should support dual sensors mapping to the same note") {
      auto result_a = lookup.FindMidiNote(1, 0);
      auto result_b = lookup.FindMidiNote(1, 1);
      REQUIRE(result_a.has_value());
      REQUIRE(result_b.has_value());
      REQUIRE(*result_a == *result_b);
    }

    SECTION("Should reflect live data changes") {
      data.entries[3] = {.board_id = 4, .sensor_id = 10, .midi_note = 72};
      data.entry_count = 4;

      auto result = lookup.FindMidiNote(4, 10);
      REQUIRE(result.has_value());
      REQUIRE(*result == 72);
    }
  }
}

#endif
