#if defined(UNIT_TESTS)

#include "midi-monitor/midi_activity_tap.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

#include "midi-monitor/midi_activity_collector.hpp"
#include "midi/midi_fanout_controller.hpp"

namespace {

using midismith::midi_monitor::MidiActivitySource;

struct RecordingMidiController final : public midismith::midi::MidiControllerRequirements {
  std::vector<std::vector<std::uint8_t>> received_messages;

  void SendRawMessage(const std::uint8_t* data, std::uint8_t length) noexcept override {
    received_messages.emplace_back(data, data + length);
  }
};

struct RecordedActivity {
  MidiActivitySource source;
  std::vector<std::uint8_t> message;
};

struct RecordingActivityRecorder final
    : public midismith::midi_monitor::MidiActivityRecorderRequirements {
  std::vector<RecordedActivity> recorded;

  void RecordMessage(MidiActivitySource source, const std::uint8_t* data,
                     std::uint8_t length) noexcept override {
    recorded.push_back(RecordedActivity{source, std::vector<std::uint8_t>(data, data + length)});
  }
};

}  // namespace

TEST_CASE("The MidiActivityTap class") {
  RecordingMidiController sink;
  RecordingActivityRecorder recorder;

  SECTION("The SendRawMessage() method") {
    SECTION("When a message passes through") {
      SECTION("Should forward the very same bytes to the sink") {
        midismith::midi_monitor::MidiActivityTap tap(sink, recorder, MidiActivitySource::kKeys);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        tap.SendRawMessage(note_on, 3);

        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }

      SECTION("Should record it under the configured source") {
        midismith::midi_monitor::MidiActivityTap tap(sink, recorder, MidiActivitySource::kDinIn);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        tap.SendRawMessage(note_on, 3);

        REQUIRE(recorder.recorded.size() == 1);
        REQUIRE(recorder.recorded[0].source == MidiActivitySource::kDinIn);
        REQUIRE(recorder.recorded[0].message == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }
    }

    SECTION("When several messages pass through") {
      SECTION("Should forward and record every one of them in order") {
        midismith::midi_monitor::MidiActivityTap tap(sink, recorder, MidiActivitySource::kUsbOut);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        const std::uint8_t note_off[] = {0x80, 0x3C, 0x00};
        const std::uint8_t clock[] = {0xF8};

        tap.SendRawMessage(note_on, 3);
        tap.SendRawMessage(note_off, 3);
        tap.SendRawMessage(clock, 1);

        REQUIRE(sink.received_messages.size() == 3);
        REQUIRE(recorder.recorded.size() == 3);
        REQUIRE(sink.received_messages[2] == std::vector<std::uint8_t>{0xF8});
        REQUIRE(recorder.recorded[2].message == std::vector<std::uint8_t>{0xF8});
      }
    }

    SECTION("When two taps share one recorder") {
      SECTION("Should attribute each message to its own source") {
        RecordingMidiController other_sink;
        midismith::midi_monitor::MidiActivityTap keys_tap(sink, recorder,
                                                          MidiActivitySource::kKeys);
        midismith::midi_monitor::MidiActivityTap din_tap(other_sink, recorder,
                                                         MidiActivitySource::kDinIn);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        keys_tap.SendRawMessage(note_on, 3);
        din_tap.SendRawMessage(note_on, 3);

        REQUIRE(recorder.recorded.size() == 2);
        REQUIRE(recorder.recorded[0].source == MidiActivitySource::kKeys);
        REQUIRE(recorder.recorded[1].source == MidiActivitySource::kDinIn);
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(other_sink.received_messages.size() == 1);
      }
    }

    SECTION("When the sink is a fanout controller") {
      SECTION("Should reach every fanout sink and still record once") {
        RecordingMidiController first;
        RecordingMidiController second;
        midismith::midi::MidiControllerRequirements* sinks[] = {&first, &second};
        midismith::midi::MidiFanoutController fanout(sinks, 2);
        midismith::midi_monitor::MidiActivityTap tap(fanout, recorder, MidiActivitySource::kKeys);

        const std::uint8_t note_on[] = {0x90, 0x3C, 0x7F};
        tap.SendRawMessage(note_on, 3);

        REQUIRE(first.received_messages.size() == 1);
        REQUIRE(second.received_messages.size() == 1);
        REQUIRE(recorder.recorded.size() == 1);
      }
    }

    SECTION("When the recorder is a collector") {
      SECTION("Should let the note reach the snapshot") {
        midismith::midi_monitor::MidiActivityCollector collector;
        midismith::midi_monitor::MidiActivityTap tap(sink, collector, MidiActivitySource::kDinIn);

        const std::uint8_t note_on[] = {0x95, 0x3D, 0x70};
        tap.SendRawMessage(note_on, 3);

        const auto snapshot = collector.CaptureSnapshot();

        REQUIRE(snapshot.has_last_note);
        REQUIRE(snapshot.last_note_number == 0x3D);
        REQUIRE(snapshot.last_note_velocity == 0x70);
        REQUIRE(snapshot.last_note_channel == 5);
        REQUIRE(snapshot.message_count(MidiActivitySource::kDinIn) == 1);
      }
    }
  }
}

#endif
