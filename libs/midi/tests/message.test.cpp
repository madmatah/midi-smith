#if defined(UNIT_TESTS)

#include "midi/message.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

namespace {

using midismith::midi::MessageKind;

}  // namespace

TEST_CASE("The IsStatusByte function") {
  SECTION("When the high bit is set") {
    SECTION("Should report a status byte") {
      REQUIRE(midismith::midi::IsStatusByte(0x80));
      REQUIRE(midismith::midi::IsStatusByte(0x90));
      REQUIRE(midismith::midi::IsStatusByte(0xFF));
    }
  }

  SECTION("When the high bit is clear") {
    SECTION("Should report a data byte") {
      REQUIRE_FALSE(midismith::midi::IsStatusByte(0x00));
      REQUIRE_FALSE(midismith::midi::IsStatusByte(0x3C));
      REQUIRE_FALSE(midismith::midi::IsStatusByte(0x7F));
    }
  }
}

TEST_CASE("The KindOf function") {
  SECTION("When given a channel voice status") {
    SECTION("Should classify every channel voice family") {
      REQUIRE(midismith::midi::KindOf(0x80) == MessageKind::kNoteOff);
      REQUIRE(midismith::midi::KindOf(0x90) == MessageKind::kNoteOn);
      REQUIRE(midismith::midi::KindOf(0xA0) == MessageKind::kPolyAftertouch);
      REQUIRE(midismith::midi::KindOf(0xB0) == MessageKind::kControlChange);
      REQUIRE(midismith::midi::KindOf(0xC0) == MessageKind::kProgramChange);
      REQUIRE(midismith::midi::KindOf(0xD0) == MessageKind::kChannelAftertouch);
      REQUIRE(midismith::midi::KindOf(0xE0) == MessageKind::kPitchBend);
    }

    SECTION("Should ignore the channel nibble") {
      REQUIRE(midismith::midi::KindOf(0x9F) == MessageKind::kNoteOn);
      REQUIRE(midismith::midi::KindOf(0x8A) == MessageKind::kNoteOff);
      REQUIRE(midismith::midi::KindOf(0xB7) == MessageKind::kControlChange);
    }
  }

  SECTION("When given a system common status") {
    SECTION("Should classify it as system common") {
      REQUIRE(midismith::midi::KindOf(0xF0) == MessageKind::kSystemCommon);
      REQUIRE(midismith::midi::KindOf(0xF2) == MessageKind::kSystemCommon);
      REQUIRE(midismith::midi::KindOf(0xF7) == MessageKind::kSystemCommon);
    }
  }

  SECTION("When given a system realtime status") {
    SECTION("Should classify it as system realtime") {
      REQUIRE(midismith::midi::KindOf(0xF8) == MessageKind::kSystemRealtime);
      REQUIRE(midismith::midi::KindOf(0xFA) == MessageKind::kSystemRealtime);
      REQUIRE(midismith::midi::KindOf(0xFF) == MessageKind::kSystemRealtime);
    }
  }

  SECTION("When given a data byte") {
    SECTION("Should classify it as unknown") {
      REQUIRE(midismith::midi::KindOf(0x00) == MessageKind::kUnknown);
      REQUIRE(midismith::midi::KindOf(0x3C) == MessageKind::kUnknown);
      REQUIRE(midismith::midi::KindOf(0x7F) == MessageKind::kUnknown);
    }
  }
}

TEST_CASE("The ChannelOf function") {
  SECTION("When given a channel voice status") {
    SECTION("Should extract the channel nibble") {
      REQUIRE(midismith::midi::ChannelOf(0x90) == 0);
      REQUIRE(midismith::midi::ChannelOf(0x91) == 1);
      REQUIRE(midismith::midi::ChannelOf(0x9F) == 15);
      REQUIRE(midismith::midi::ChannelOf(0x8A) == 10);
    }
  }

  SECTION("When given a system status that carries no channel") {
    SECTION("Should report channel zero rather than the low nibble") {
      REQUIRE(midismith::midi::ChannelOf(0xF8) == 0);
      REQUIRE(midismith::midi::ChannelOf(0xFA) == 0);
      REQUIRE(midismith::midi::ChannelOf(0xF2) == 0);
    }
  }

  SECTION("When given a data byte") {
    SECTION("Should report channel zero") {
      REQUIRE(midismith::midi::ChannelOf(0x3C) == 0);
    }
  }
}

TEST_CASE("The StartsNote function") {
  SECTION("When given a note-on with a positive velocity") {
    SECTION("Should report that a note starts") {
      REQUIRE(midismith::midi::StartsNote(0x90, 1));
      REQUIRE(midismith::midi::StartsNote(0x9F, 127));
    }
  }

  SECTION("When given a note-on with a zero velocity") {
    SECTION("Should report that no note starts") {
      REQUIRE_FALSE(midismith::midi::StartsNote(0x90, 0));
    }
  }

  SECTION("When given a note-off") {
    SECTION("Should report that no note starts") {
      REQUIRE_FALSE(midismith::midi::StartsNote(0x80, 64));
      REQUIRE_FALSE(midismith::midi::StartsNote(0x80, 0));
    }
  }

  SECTION("When given a message that is not a note") {
    SECTION("Should report that no note starts") {
      REQUIRE_FALSE(midismith::midi::StartsNote(0xB0, 127));
      REQUIRE_FALSE(midismith::midi::StartsNote(0xF8, 127));
    }
  }
}

TEST_CASE("The ReleasesNote function") {
  SECTION("When given a note-off") {
    SECTION("Should report that a note is released") {
      REQUIRE(midismith::midi::ReleasesNote(0x80, 0));
      REQUIRE(midismith::midi::ReleasesNote(0x8F, 64));
    }
  }

  SECTION("When given a note-on with a zero velocity") {
    SECTION("Should report that a note is released") {
      REQUIRE(midismith::midi::ReleasesNote(0x90, 0));
      REQUIRE(midismith::midi::ReleasesNote(0x9F, 0));
    }
  }

  SECTION("When given a note-on with a positive velocity") {
    SECTION("Should report that no note is released") {
      REQUIRE_FALSE(midismith::midi::ReleasesNote(0x90, 1));
      REQUIRE_FALSE(midismith::midi::ReleasesNote(0x90, 127));
    }
  }

  SECTION("When given a message that is not a note") {
    SECTION("Should report that no note is released") {
      REQUIRE_FALSE(midismith::midi::ReleasesNote(0xB0, 0));
      REQUIRE_FALSE(midismith::midi::ReleasesNote(0xF8, 0));
    }
  }
}

#endif
