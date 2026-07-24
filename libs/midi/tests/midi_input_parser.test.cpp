#if defined(UNIT_TESTS)

#include "midi/midi_input_parser.hpp"

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

void FeedAll(midismith::midi::MidiInputParser& parser, std::initializer_list<std::uint8_t> bytes) {
  for (auto byte : bytes) {
    parser.Feed(byte);
  }
}

}  // namespace

TEST_CASE("MidiInputParser") {
  RecordingMidiController sink;
  midismith::midi::MidiInputParser parser(sink);

  SECTION("Feed()") {
    SECTION("When receiving a complete NoteOn (90 3C 7F)") {
      SECTION("Should emit one 3-byte message") {
        FeedAll(parser, {0x90, 0x3C, 0x7F});
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }
    }

    SECTION("When receiving a complete Program Change (C0 05)") {
      SECTION("Should emit one 2-byte message") {
        FeedAll(parser, {0xC0, 0x05});
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xC0, 0x05});
      }
    }

    SECTION("When receiving a Tune Request (F6)") {
      SECTION("Should emit one 1-byte message immediately") {
        parser.Feed(0xF6);
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xF6});
      }
    }

    SECTION("When receiving running status (90 3C 7F 40 7F)") {
      SECTION("Should emit two NoteOn messages with the same status") {
        FeedAll(parser, {0x90, 0x3C, 0x7F, 0x40, 0x7F});
        REQUIRE(sink.received_messages.size() == 2);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(sink.received_messages[1] == std::vector<std::uint8_t>{0x90, 0x40, 0x7F});
      }
    }

    SECTION("When receiving a single Clock byte (F8)") {
      SECTION("Should emit it immediately as a 1-byte message") {
        parser.Feed(0xF8);
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xF8});
      }
    }

    SECTION("When a Clock byte is interleaved mid-NoteOn (90 3C F8 7F)") {
      SECTION("Should emit Clock first, then the complete NoteOn") {
        FeedAll(parser, {0x90, 0x3C, 0xF8, 0x7F});
        REQUIRE(sink.received_messages.size() == 2);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xF8});
        REQUIRE(sink.received_messages[1] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }
    }

    SECTION("When receiving data bytes without prior status") {
      SECTION("Should ignore them") {
        FeedAll(parser, {0x3C, 0x7F, 0x40});
        REQUIRE(sink.received_messages.empty());
      }
    }

    SECTION("When a status byte arrives mid-message (90 3C 80 3D 40)") {
      SECTION("Should discard the partial NoteOn and emit the NoteOff") {
        FeedAll(parser, {0x90, 0x3C, 0x80, 0x3D, 0x40});
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x80, 0x3D, 0x40});
      }
    }

    SECTION("System exclusive is deliberately dropped and never relayed") {
      SECTION(
          "Because the sinks carry whole messages of at most three bytes, a dump would have "
          "to be split, and a dump truncated by a full queue leaves the receiver waiting "
          "for an end-of-exclusive that never comes") {
        SECTION("When receiving a complete SysEx (F0 01 02 03 F7)") {
          SECTION("Should emit nothing") {
            FeedAll(parser, {0xF0, 0x01, 0x02, 0x03, 0xF7});
            REQUIRE(sink.received_messages.empty());
          }
        }

        SECTION("When data bytes follow a SysEx that interrupted running status") {
          SECTION("Should ignore them because SysEx clears running status") {
            FeedAll(parser, {0x90, 0x3C, 0x7F});
            REQUIRE(sink.received_messages.size() == 1);

            FeedAll(parser, {0xF0, 0x01, 0xF7});
            REQUIRE(sink.received_messages.size() == 1);

            FeedAll(parser, {0x3D, 0x7F});
            REQUIRE(sink.received_messages.size() == 1);
          }
        }

        SECTION("When a Clock byte appears inside a SysEx") {
          SECTION("Should emit the Clock and continue dropping SysEx") {
            FeedAll(parser, {0xF0, 0x01, 0xF8, 0x02, 0xF7});
            REQUIRE(sink.received_messages.size() == 1);
            REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xF8});
          }
        }

        SECTION("When a new SysEx starts while another is in progress") {
          SECTION("Should restart the SysEx silently") {
            FeedAll(parser, {0xF0, 0x01, 0xF0, 0x02, 0xF7});
            REQUIRE(sink.received_messages.empty());
          }
        }

        SECTION("When a channel voice status interrupts a SysEx") {
          SECTION("Should abort the SysEx and start the new message") {
            FeedAll(parser, {0xF0, 0x01, 0x02, 0x90, 0x3C, 0x7F});
            REQUIRE(sink.received_messages.size() == 1);
            REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
          }
        }
      }
    }

    SECTION("When two complete messages arrive back-to-back (90 3C 7F B0 40 7F)") {
      SECTION("Should emit both in order") {
        FeedAll(parser, {0x90, 0x3C, 0x7F, 0xB0, 0x40, 0x7F});
        REQUIRE(sink.received_messages.size() == 2);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(sink.received_messages[1] == std::vector<std::uint8_t>{0xB0, 0x40, 0x7F});
      }
    }

    SECTION("When a Song Position Pointer (F2 LSB MSB) arrives") {
      SECTION("Should emit a 3-byte message") {
        FeedAll(parser, {0xF2, 0x10, 0x20});
        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0xF2, 0x10, 0x20});
      }
    }

    SECTION("When system common (F1) clears running status, subsequent data is ignored") {
      SECTION("Should not reuse the previous channel voice status") {
        FeedAll(parser, {0x90, 0x3C, 0x7F});
        FeedAll(parser, {0xF1, 0x05});
        FeedAll(parser, {0x3D, 0x7F});
        REQUIRE(sink.received_messages.size() == 2);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
        REQUIRE(sink.received_messages[1] == std::vector<std::uint8_t>{0xF1, 0x05});
      }
    }

    SECTION("When a stray EOX (F7) lands in the middle of a channel message") {
      SECTION("Should abort the pending message instead of completing it across the EOX") {
        FeedAll(parser, {0x90, 0x3C, 0xF7, 0x40});

        REQUIRE(sink.received_messages.empty());
      }

      SECTION("Should also drop the running status") {
        FeedAll(parser, {0x90, 0x3C, 0x7F});
        FeedAll(parser, {0xF7});
        FeedAll(parser, {0x3D, 0x7F});

        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3C, 0x7F});
      }

      SECTION("Should let a fresh message parse normally afterwards") {
        FeedAll(parser, {0x90, 0x3C, 0xF7});
        FeedAll(parser, {0x90, 0x3E, 0x64});

        REQUIRE(sink.received_messages.size() == 1);
        REQUIRE(sink.received_messages[0] == std::vector<std::uint8_t>{0x90, 0x3E, 0x64});
      }
    }
  }
}

#endif
