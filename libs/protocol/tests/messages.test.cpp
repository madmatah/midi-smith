#if defined(UNIT_TESTS)

#include "protocol/messages.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

using namespace midismith::protocol;

TEST_CASE("The SensorEvent struct") {
  SECTION("The Serialize() method") {
    SECTION("When the output buffer has at least 3 bytes") {
      SECTION("Should write the event type, note index, and velocity in order") {
        SensorEvent event{SensorEventType::kNoteOn, 60, 100};
        std::array<std::uint8_t, 3> buffer{};

        REQUIRE(event.Serialize(buffer) == true);
        REQUIRE(buffer[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOn));
        REQUIRE(buffer[1] == 60);
        REQUIRE(buffer[2] == 100);
      }
    }

    SECTION("When the output buffer is smaller than 3 bytes") {
      SECTION("Should return false") {
        SensorEvent event{SensorEventType::kNoteOn, 60, 100};
        std::array<std::uint8_t, 2> buffer{};

        REQUIRE(event.Serialize(buffer) == false);
      }
    }
  }
}

TEST_CASE("The Heartbeat struct") {
  SECTION("The Serialize() method") {
    SECTION("When the output buffer has at least 1 byte") {
      SECTION("Should write the device state byte") {
        Heartbeat hb{0x02};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE(hb.Serialize(buffer) == true);
        REQUIRE(buffer[0] == 0x02);
      }
    }
  }
}

TEST_CASE("The Command struct") {
  SECTION("The Serialize() method") {
    SECTION("When the output buffer has at least 2 bytes") {
      SECTION("Should write the action code and parameter in order") {
        Command cmd{0x03, 0x01};
        std::array<std::uint8_t, 2> buffer{};

        REQUIRE(cmd.Serialize(buffer) == true);
        REQUIRE(buffer[0] == 0x03);
        REQUIRE(buffer[1] == 0x01);
      }
    }
  }
}

#endif
