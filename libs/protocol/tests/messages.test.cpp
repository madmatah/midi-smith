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
        Heartbeat hb{DeviceState::kCalibrating};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE(hb.Serialize(buffer) == true);
        REQUIRE(buffer[0] == static_cast<std::uint8_t>(DeviceState::kCalibrating));
      }
    }

    SECTION("When the output buffer is empty") {
      SECTION("Should return false") {
        Heartbeat hb{DeviceState::kIdle};
        std::array<std::uint8_t, 0> buffer{};

        REQUIRE(hb.Serialize(buffer) == false);
      }
    }
  }
}

TEST_CASE("The Command variant — Serialize()") {
  SECTION("AdcStart serializes to a single byte 0x01") {
    Command cmd = AdcStart{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == true);
    REQUIRE(buffer[0] == 0x01);
  }

  SECTION("AdcStop serializes to a single byte 0x02") {
    Command cmd = AdcStop{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == true);
    REQUIRE(buffer[0] == 0x02);
  }

  SECTION("CalibStart serializes to action byte 0x03 followed by the mode byte") {
    SECTION("With kAuto mode") {
      Command cmd = CalibStart{CalibMode::kAuto};
      std::array<std::uint8_t, 2> buffer{};

      REQUIRE(Serialize(cmd, buffer) == true);
      REQUIRE(buffer[0] == 0x03);
      REQUIRE(buffer[1] == static_cast<std::uint8_t>(CalibMode::kAuto));
    }

    SECTION("With kManual mode") {
      Command cmd = CalibStart{CalibMode::kManual};
      std::array<std::uint8_t, 2> buffer{};

      REQUIRE(Serialize(cmd, buffer) == true);
      REQUIRE(buffer[0] == 0x03);
      REQUIRE(buffer[1] == static_cast<std::uint8_t>(CalibMode::kManual));
    }

    SECTION("When the output buffer is smaller than 2 bytes") {
      SECTION("Should return false") {
        Command cmd = CalibStart{CalibMode::kAuto};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE(Serialize(cmd, buffer) == false);
      }
    }
  }

  SECTION("DumpRequest serializes to a single byte 0x04") {
    Command cmd = DumpRequest{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == true);
    REQUIRE(buffer[0] == 0x04);
  }
}

#endif
