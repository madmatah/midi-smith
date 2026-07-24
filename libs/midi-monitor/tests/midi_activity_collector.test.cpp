#if defined(UNIT_TESTS)

#include "midi-monitor/midi_activity_collector.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

using midismith::midi_monitor::MidiActivityCollector;
using midismith::midi_monitor::MidiActivitySource;

void Record(MidiActivityCollector& collector, MidiActivitySource source,
            std::initializer_list<std::uint8_t> bytes) {
  const std::vector<std::uint8_t> message(bytes);
  collector.RecordMessage(source, message.data(), static_cast<std::uint8_t>(message.size()));
}

void RecordNoteOn(MidiActivityCollector& collector, std::uint8_t channel, std::uint8_t note,
                  std::uint8_t velocity, MidiActivitySource source = MidiActivitySource::kKeys) {
  Record(collector, source, {static_cast<std::uint8_t>(0x90 | channel), note, velocity});
}

void RecordNoteOff(MidiActivityCollector& collector, std::uint8_t channel, std::uint8_t note,
                   MidiActivitySource source = MidiActivitySource::kKeys) {
  Record(collector, source, {static_cast<std::uint8_t>(0x80 | channel), note, 0x40});
}

}  // namespace

TEST_CASE("The MidiActivityCollector class") {
  MidiActivityCollector collector;

  SECTION("The CaptureSnapshot() method") {
    SECTION("When nothing has been recorded") {
      SECTION("Should report an empty snapshot") {
        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE_FALSE(snapshot.has_last_note);
        REQUIRE(snapshot.active_note_count == 0);
        REQUIRE(snapshot.total_message_count == 0);
        REQUIRE(snapshot.last_note_sequence == 0);
      }
    }
  }

  SECTION("The RecordMessage() method") {
    SECTION("When a note-on is recorded") {
      SECTION("Should expose it as the last note") {
        RecordNoteOn(collector, 5, 61, 112);

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.has_last_note);
        REQUIRE(snapshot.last_note_number == 61);
        REQUIRE(snapshot.last_note_velocity == 112);
        REQUIRE(snapshot.last_note_channel == 5);
        REQUIRE(snapshot.active_note_count == 1);
      }

      SECTION("Should advance the sequence on every note-on") {
        RecordNoteOn(collector, 0, 60, 100);
        const auto first = collector.CaptureSnapshot();

        RecordNoteOn(collector, 0, 60, 100);
        const auto second = collector.CaptureSnapshot();

        REQUIRE(first.last_note_sequence != second.last_note_sequence);
      }

      SECTION("Should preserve every field of the highest channel and velocity") {
        RecordNoteOn(collector, 15, 127, 127);

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.last_note_number == 127);
        REQUIRE(snapshot.last_note_velocity == 127);
        REQUIRE(snapshot.last_note_channel == 15);
      }
    }

    SECTION("When a note-on carries a zero velocity") {
      SECTION("Should release the note instead of starting it") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOn(collector, 0, 60, 0);

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.active_note_count == 0);
      }

      SECTION("Should not become the last note") {
        RecordNoteOn(collector, 0, 60, 100);
        const auto before = collector.CaptureSnapshot();

        RecordNoteOn(collector, 0, 72, 0);
        const auto after = collector.CaptureSnapshot();

        REQUIRE(after.last_note_number == before.last_note_number);
        REQUIRE(after.last_note_sequence == before.last_note_sequence);
      }
    }

    SECTION("When an explicit note-off is recorded") {
      SECTION("Should release the note") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOff(collector, 0, 60);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 0);
      }

      SECTION("Should keep the released note as the last note") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOff(collector, 0, 60);

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.has_last_note);
        REQUIRE(snapshot.last_note_number == 60);
      }
    }

    SECTION("When notes span the whole bitmap") {
      SECTION("Should count them across every bitmap word") {
        RecordNoteOn(collector, 0, 0, 100);
        RecordNoteOn(collector, 0, 31, 100);
        RecordNoteOn(collector, 0, 32, 100);
        RecordNoteOn(collector, 0, 64, 100);
        RecordNoteOn(collector, 0, 96, 100);
        RecordNoteOn(collector, 0, 127, 100);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 6);
      }

      SECTION("Should release each one independently") {
        RecordNoteOn(collector, 0, 31, 100);
        RecordNoteOn(collector, 0, 32, 100);
        RecordNoteOff(collector, 0, 31);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 1);
      }
    }

    SECTION("When the same note is pressed twice without release") {
      SECTION("Should count it once") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOn(collector, 0, 60, 100);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 1);
      }
    }

    SECTION("When the same note is released twice") {
      SECTION("Should not underflow the count") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOff(collector, 0, 60);
        RecordNoteOff(collector, 0, 60);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 0);
      }
    }

    SECTION("When a note that was never pressed is released") {
      SECTION("Should leave the count at zero") {
        RecordNoteOff(collector, 0, 60);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 0);
      }
    }

    SECTION("When the same note is held on two channels") {
      SECTION("Should count one key down") {
        RecordNoteOn(collector, 0, 60, 100);
        RecordNoteOn(collector, 1, 60, 100);

        REQUIRE(collector.CaptureSnapshot().active_note_count == 1);
      }
    }

    SECTION("When a message that is not a note is recorded") {
      SECTION("Should count it without touching the notes") {
        RecordNoteOn(collector, 0, 60, 100);
        Record(collector, MidiActivitySource::kKeys, {0xB0, 0x40, 0x7F});

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.active_note_count == 1);
        REQUIRE(snapshot.last_note_number == 60);
        REQUIRE(snapshot.message_count(MidiActivitySource::kKeys) == 2);
      }
    }

    SECTION("When a single byte realtime message is recorded") {
      SECTION("Should count it without touching the notes") {
        Record(collector, MidiActivitySource::kDinIn, {0xF8});

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.message_count(MidiActivitySource::kDinIn) == 1);
        REQUIRE(snapshot.active_note_count == 0);
        REQUIRE_FALSE(snapshot.has_last_note);
      }
    }

    SECTION("When messages come from different sources") {
      SECTION("Should count them separately and total them") {
        RecordNoteOn(collector, 0, 60, 100, MidiActivitySource::kKeys);
        RecordNoteOn(collector, 0, 62, 100, MidiActivitySource::kDinIn);
        RecordNoteOn(collector, 0, 64, 100, MidiActivitySource::kDinIn);
        Record(collector, MidiActivitySource::kUsbOut, {0xF8});

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.message_count(MidiActivitySource::kKeys) == 1);
        REQUIRE(snapshot.message_count(MidiActivitySource::kDinIn) == 2);
        REQUIRE(snapshot.message_count(MidiActivitySource::kUsbOut) == 1);
        REQUIRE(snapshot.message_count(MidiActivitySource::kDinOut) == 0);
        REQUIRE(snapshot.total_message_count == 4);
      }
    }

    SECTION("When the message is empty or absent") {
      SECTION("Should ignore it") {
        collector.RecordMessage(MidiActivitySource::kKeys, nullptr, 3);
        const std::uint8_t note_on[] = {0x90, 0x3C, 0x64};
        collector.RecordMessage(MidiActivitySource::kKeys, note_on, 0);

        REQUIRE(collector.CaptureSnapshot().total_message_count == 0);
      }
    }
  }
}

#endif
