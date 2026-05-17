#if defined(UNIT_TESTS)

#include "midi/midi_fanout_controller.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace {

struct RecordingMidiController final : public midismith::midi::MidiControllerRequirements {
  std::vector<std::vector<std::uint8_t>> received_messages;

  void SendRawMessage(const std::uint8_t* data, std::uint8_t length) noexcept override {
    received_messages.emplace_back(data, data + length);
  }
};

}  // namespace

TEST_CASE("MidiFanoutController") {
  SECTION("SendRawMessage()") {
    SECTION("When the sinks list is empty") {
      SECTION("Should not crash") {
        midismith::midi::MidiFanoutController fanout(nullptr, 0);
        const std::uint8_t message[] = {0x90, 0x3C, 0x7F};
        fanout.SendRawMessage(message, 3);
      }
    }

    SECTION("When the sinks list contains two controllers") {
      SECTION("Should forward the same message to each sink, in order") {
        RecordingMidiController first;
        RecordingMidiController second;
        midismith::midi::MidiControllerRequirements* sinks[] = {&first, &second};
        midismith::midi::MidiFanoutController fanout(sinks, 2);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        fanout.SendRawMessage(note_on, 3);

        REQUIRE(first.received_messages.size() == 1);
        REQUIRE(second.received_messages.size() == 1);
        REQUIRE(first.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(second.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }
    }

    SECTION("When called multiple times with different messages") {
      SECTION("Should deliver every message to every sink in arrival order") {
        RecordingMidiController first;
        RecordingMidiController second;
        midismith::midi::MidiControllerRequirements* sinks[] = {&first, &second};
        midismith::midi::MidiFanoutController fanout(sinks, 2);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        const std::uint8_t note_off[] = {0x80, 0x3C, 0x00};
        const std::uint8_t cc[] = {0xB0, 0x40, 0x7F};

        fanout.SendRawMessage(note_on, 3);
        fanout.SendRawMessage(note_off, 3);
        fanout.SendRawMessage(cc, 3);

        REQUIRE(first.received_messages.size() == 3);
        REQUIRE(second.received_messages.size() == 3);
        REQUIRE(first.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(first.received_messages[1] == std::vector<std::uint8_t>{0x80, 0x3C, 0x00});
        REQUIRE(first.received_messages[2] == std::vector<std::uint8_t>{0xB0, 0x40, 0x7F});
        REQUIRE(second.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(second.received_messages[1] == std::vector<std::uint8_t>{0x80, 0x3C, 0x00});
        REQUIRE(second.received_messages[2] == std::vector<std::uint8_t>{0xB0, 0x40, 0x7F});
      }
    }

    SECTION("When a sink slot is null") {
      SECTION("Should skip it and still deliver to the other sinks") {
        RecordingMidiController first;
        RecordingMidiController third;
        midismith::midi::MidiControllerRequirements* sinks[] = {&first, nullptr, &third};
        midismith::midi::MidiFanoutController fanout(sinks, 3);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        fanout.SendRawMessage(note_on, 3);

        REQUIRE(first.received_messages.size() == 1);
        REQUIRE(third.received_messages.size() == 1);
      }
    }
  }
}

#endif
