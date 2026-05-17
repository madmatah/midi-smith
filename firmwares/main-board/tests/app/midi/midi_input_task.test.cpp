#if defined(UNIT_TESTS)

#include "app/midi/midi_input_task.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <deque>
#include <vector>

namespace {

struct FixedReadableStreamStub final : public midismith::io::ReadableStreamRequirements {
  std::deque<std::uint8_t> bytes_to_deliver;

  midismith::io::ReadResult Read(std::uint8_t& byte) noexcept override {
    if (bytes_to_deliver.empty()) {
      return midismith::io::ReadResult::kNoData;
    }
    byte = bytes_to_deliver.front();
    bytes_to_deliver.pop_front();
    return midismith::io::ReadResult::kOk;
  }
};

struct RecordingMidiController final : public midismith::midi::MidiControllerRequirements {
  std::vector<std::vector<std::uint8_t>> received_messages;

  void SendRawMessage(const std::uint8_t* data, std::uint8_t length) noexcept override {
    received_messages.emplace_back(data, data + length);
  }
};

struct ScriptedSemaphore final : public midismith::os::BinarySemaphoreRequirements {
  std::uint32_t remaining_acquires = 0;

  bool Acquire(std::uint32_t /*timeout_ms*/) noexcept override {
    if (remaining_acquires == 0) {
      return false;
    }
    remaining_acquires--;
    return true;
  }

  bool Release() noexcept override {
    return true;
  }
};

}  // namespace

TEST_CASE("MidiInputTask") {
  FixedReadableStreamStub source;
  RecordingMidiController sink;
  ScriptedSemaphore wake_signal;
  midismith::main_board::app::midi::MidiInputTask task(source, sink, wake_signal);

  SECTION("Run()") {
    SECTION("When the source has bytes ready for one wake cycle") {
      SECTION("Should drain all bytes and forward complete messages to the sink") {
        source.bytes_to_deliver = {0x90, 0x3C, 0x7F};
        wake_signal.remaining_acquires = 1;

        task.Run();

        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }
    }

    SECTION("When the wake signal fires multiple times across messages") {
      SECTION("Should drain across all wake cycles") {
        source.bytes_to_deliver = {0x90, 0x3C, 0x7F, 0xB0, 0x40, 0x7F};
        wake_signal.remaining_acquires = 2;

        task.Run();

        REQUIRE(sink.received_messages.size() == 2);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(sink.received_messages[1] == std::vector<std::uint8_t>{0xB0, 0x40, 0x7F});
      }
    }

    SECTION("When the source has no bytes on wake") {
      SECTION("Should not emit anything and return on next failed acquire") {
        wake_signal.remaining_acquires = 1;

        task.Run();

        REQUIRE(sink.received_messages.empty());
      }
    }

    SECTION("When the wake signal never fires") {
      SECTION("Should exit immediately") {
        source.bytes_to_deliver = {0x90, 0x3C, 0x7F};
        wake_signal.remaining_acquires = 0;

        task.Run();

        REQUIRE(sink.received_messages.empty());
      }
    }
  }
}

#endif
