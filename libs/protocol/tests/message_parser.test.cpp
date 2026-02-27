#if defined(UNIT_TESTS)

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "protocol/builders.hpp"
#include "protocol/message_parser.hpp"

using namespace midismith::protocol;

TEST_CASE("The MessageParser class") {
  SECTION("The Decode() method") {
    SECTION("When given a valid NoteEvent header and a correctly sized payload") {
      SECTION("Should reconstruct the NoteEvent from the raw bytes") {
        AdcMessageBuilder builder(4);
        auto [header, _] = builder.BuildNoteOn(60, 100);
        std::array<std::uint8_t, 3> payload = {
            static_cast<std::uint8_t>(NoteEventType::kNoteOn), 60, 100};

        auto result = MessageParser::Decode(header, payload);

        REQUIRE(result.has_value());
        auto* note_event = std::get_if<NoteEvent>(&result.value());
        REQUIRE(note_event != nullptr);
        REQUIRE(note_event->type == NoteEventType::kNoteOn);
        REQUIRE(note_event->note_index == 60);
        REQUIRE(note_event->velocity == 100);
      }
    }

    SECTION("When given a valid NoteEvent header and an undersized payload") {
      SECTION("Should return nullopt") {
        AdcMessageBuilder builder(4);
        auto [header, _] = builder.BuildNoteOn(60, 100);
        std::array<std::uint8_t, 2> payload = {1, 60};

        auto result = MessageParser::Decode(header, payload);

        REQUIRE_FALSE(result.has_value());
      }
    }

    SECTION("When given a valid Command header and a correctly sized payload") {
      SECTION("Should reconstruct the Command from the raw bytes") {
        MainBoardMessageBuilder builder;
        auto [header, _] = builder.BuildStartCalibration(4, true);
        std::array<std::uint8_t, 2> payload = {0x03, 0x00};

        auto result = MessageParser::Decode(header, payload);

        REQUIRE(result.has_value());
        auto* cmd = std::get_if<Command>(&result.value());
        REQUIRE(cmd != nullptr);
        REQUIRE(cmd->action_code == 0x03);
        REQUIRE(cmd->parameter == 0x00);
      }
    }
  }
}

#endif
