#include "domain/music/piano/midi_piano.hpp"

#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using midismith::main_board::domain::midi::MidiControllerRequirements;
using midismith::main_board::domain::music::piano::MidiPiano;

#define fakeit_Method(mock, method) Method(mock, method)

TEST_CASE("The MidiPiano class") {
  Mock<MidiControllerRequirements> midi_mock;
  MidiPiano::Config config{.channel = 5, .sustain_cc = 64, .soft_cc = 67, .sostenuto_cc = 66};
  MidiPiano piano(midi_mock.get(), config);

  SECTION("The PressKey() method") {
    SECTION("When called with middle C (60) and velocity 100") {
      SECTION("Should send a Note ON message (0x95) to the MIDI controller") {
        When(fakeit_Method(midi_mock, SendRawMessage)).Do([](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0x95);  // 0x90 | channel 5
          REQUIRE(data[1] == 60);    // Middle C
          REQUIRE(data[2] == 100);   // Velocity
        });

        piano.PressKey(60, 100);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }
  }

  SECTION("The ReleaseKey() method") {
    SECTION("When called with middle C (60)") {
      SECTION("Should send a Note OFF message (0x85) with velocity 0") {
        When(fakeit_Method(midi_mock, SendRawMessage)).Do([](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0x85);  // 0x80 | channel 5
          REQUIRE(data[1] == 60);    // Middle C
          REQUIRE(data[2] == 0);     // Velocity 0
        });

        piano.ReleaseKey(60);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }
  }

  SECTION("The SetSustain() method") {
    SECTION("When activating the pedal (true)") {
      SECTION("Should send a Control Change (0xB5) with value 127") {
        When(Method(midi_mock, SendRawMessage)).Do([&config](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0xB5);  // 0xB0 | channel 5
          REQUIRE(data[1] == config.sustain_cc);
          REQUIRE(data[2] == 127);
        });

        piano.SetSustain(true);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }

    SECTION("When deactivating the pedal (false)") {
      SECTION("Should send a Control Change (0xB5) with value 0") {
        When(Method(midi_mock, SendRawMessage)).Do([&config](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0xB5);
          REQUIRE(data[1] == config.sustain_cc);
          REQUIRE(data[2] == 0);
        });

        piano.SetSustain(false);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }
  }

  SECTION("The SetSoft() method") {
    SECTION("When activating the pedal (true)") {
      SECTION("Should send a Control Change (0xB5) to the soft CC") {
        When(Method(midi_mock, SendRawMessage)).Do([&config](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0xB5);
          REQUIRE(data[1] == config.soft_cc);
          REQUIRE(data[2] == 127);
        });

        piano.SetSoft(true);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }
  }

  SECTION("The SetSostenuto() method") {
    SECTION("When activating the pedal (true)") {
      SECTION("Should send a Control Change (0xB5) to the sostenuto CC") {
        When(Method(midi_mock, SendRawMessage)).Do([&config](const uint8_t* data, uint8_t length) {
          REQUIRE(length == 3);
          REQUIRE(data[0] == 0xB5);
          REQUIRE(data[1] == config.sostenuto_cc);
          REQUIRE(data[2] == 127);
        });

        piano.SetSostenuto(true);

        Verify(fakeit_Method(midi_mock, SendRawMessage)).Once();
      }
    }
  }
}
