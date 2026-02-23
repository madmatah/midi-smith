#include "app/midi/async_task_midi_controller.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

#include "os/queue_requirements.hpp"

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using midismith::main_board::app::midi::AsyncTaskMidiController;
using midismith::main_board::app::midi::MidiCommand;

#define fakeit_Method(mock, method) Method(mock, method)

TEST_CASE("The AsyncTaskMidiController class") {
  Mock<midismith::os::QueueRequirements<MidiCommand>> queue_mock;
  AsyncTaskMidiController controller(queue_mock.get());

  SECTION("The SendRawMessage() method") {
    SECTION("When called with a valid 3-byte MIDI message") {
      uint8_t midi_data[] = {0x90, 0x3C, 0x7F};  // Note On, Middle C, 127

      When(fakeit_Method(queue_mock, Send)).Do([](const MidiCommand& cmd, uint32_t timeout) {
        CHECK(cmd.data[0] == 0x90);
        CHECK(cmd.data[1] == 0x3C);
        CHECK(cmd.data[2] == 0x7F);
        CHECK(cmd.length == 3);
        CHECK(timeout == 0);
        return true;
      });

      SECTION("Should correctly encapsulate data into MidiCommand and post to queue") {
        controller.SendRawMessage(midi_data, 3);

        Verify(fakeit_Method(queue_mock, Send)).Once();
      }
    }

    SECTION("When called with a 1-byte MIDI message") {
      uint8_t midi_data[] = {0xFE};  // Active Sensing

      When(fakeit_Method(queue_mock, Send)).Do([](const MidiCommand& cmd, uint32_t timeout) {
        CHECK(cmd.data[0] == 0xFE);
        CHECK(cmd.length == 1);
        CHECK(timeout == 0);
        return true;
      });

      SECTION("Should correctly handle the single byte") {
        controller.SendRawMessage(midi_data, 1);

        Verify(fakeit_Method(queue_mock, Send)).Once();
      }
    }

    SECTION("When called with invalid parameters") {
      SECTION("Should do nothing if data is null") {
        controller.SendRawMessage(nullptr, 3);

        Verify(fakeit_Method(queue_mock, Send)).Never();
      }

      SECTION("Should do nothing if length is 0") {
        uint8_t dummy = 0;
        controller.SendRawMessage(&dummy, 0);

        Verify(fakeit_Method(queue_mock, Send)).Never();
      }

      SECTION("Should do nothing if length > 3") {
        uint8_t large_data[4] = {0};
        controller.SendRawMessage(large_data, 4);

        Verify(fakeit_Method(queue_mock, Send)).Never();
      }
    }
  }
}
