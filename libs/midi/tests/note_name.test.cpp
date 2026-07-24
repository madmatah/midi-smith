#if defined(UNIT_TESTS)

#include "midi/note_name.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "midi/types.hpp"

namespace {

std::string_view NameOf(midismith::midi::NoteNumber note,
                        std::array<char, midismith::midi::kNoteNameCapacity>& buffer) {
  return midismith::midi::FormatNoteName(note, buffer);
}

}  // namespace

TEST_CASE("The FormatNoteName function") {
  std::array<char, midismith::midi::kNoteNameCapacity> buffer{};

  SECTION("When given middle C") {
    SECTION("Should render it as C4") {
      REQUIRE(NameOf(midismith::midi::kNoteC4, buffer) == "C4");
    }
  }

  SECTION("When given the lowest note") {
    SECTION("Should render the negative octave") {
      REQUIRE(NameOf(0, buffer) == "C-1");
      REQUIRE(NameOf(1, buffer) == "C#-1");
      REQUIRE(NameOf(11, buffer) == "B-1");
    }
  }

  SECTION("When given the highest note") {
    SECTION("Should render it as G9") {
      REQUIRE(NameOf(127, buffer) == "G9");
    }
  }

  SECTION("When walking a full octave") {
    SECTION("Should render the twelve pitch classes with sharps") {
      REQUIRE(NameOf(60, buffer) == "C4");
      REQUIRE(NameOf(61, buffer) == "C#4");
      REQUIRE(NameOf(62, buffer) == "D4");
      REQUIRE(NameOf(63, buffer) == "D#4");
      REQUIRE(NameOf(64, buffer) == "E4");
      REQUIRE(NameOf(65, buffer) == "F4");
      REQUIRE(NameOf(66, buffer) == "F#4");
      REQUIRE(NameOf(67, buffer) == "G4");
      REQUIRE(NameOf(68, buffer) == "G#4");
      REQUIRE(NameOf(69, buffer) == "A4");
      REQUIRE(NameOf(70, buffer) == "A#4");
      REQUIRE(NameOf(71, buffer) == "B4");
    }
  }

  SECTION("When given the range of an 88-key piano") {
    SECTION("Should render its first and last keys") {
      REQUIRE(NameOf(21, buffer) == "A0");
      REQUIRE(NameOf(108, buffer) == "C8");
    }
  }

  SECTION("When crossing an octave boundary") {
    SECTION("Should increment the octave on C") {
      REQUIRE(NameOf(59, buffer) == "B3");
      REQUIRE(NameOf(60, buffer) == "C4");
    }
  }

  SECTION("When given a value beyond the seven-bit note range") {
    SECTION("Should mask it to a data byte") {
      REQUIRE(NameOf(128, buffer) == NameOf(0, buffer));
      REQUIRE(NameOf(255, buffer) == NameOf(127, buffer));
    }
  }

  SECTION("Whatever the note") {
    SECTION("Should never write past the buffer capacity") {
      for (int note = 0; note < 256; note++) {
        const std::string_view name =
            NameOf(static_cast<midismith::midi::NoteNumber>(note), buffer);
        REQUIRE(name.size() >= 2);
        REQUIRE(name.size() <= midismith::midi::kNoteNameCapacity);
      }
    }
  }
}

#endif
