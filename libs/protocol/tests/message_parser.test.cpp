#if defined(UNIT_TESTS)

#include "protocol/message_parser.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

#include "protocol/builders.hpp"

using namespace midismith::protocol;

TEST_CASE("The MessageParser class") {
  SECTION("The Decode() method") {
    SECTION("When given a valid SensorEvent header and a correctly sized payload") {
      SECTION("Should reconstruct the SensorEvent from the raw bytes") {
        AdcMessageBuilder builder(4);
        auto [header, _] = builder.BuildNoteOn(60, 100);
        std::array<std::uint8_t, 3> payload = {static_cast<std::uint8_t>(SensorEventType::kNoteOn),
                                               60, 100};

        auto result = MessageParser::Decode(header, payload);

        REQUIRE(result.has_value());
        auto* note_event = std::get_if<SensorEvent>(&result.value());
        REQUIRE(note_event != nullptr);
        REQUIRE(note_event->type == SensorEventType::kNoteOn);
        REQUIRE(note_event->sensor_id == 60);
        REQUIRE(note_event->velocity == 100);
      }
    }

    SECTION("When given a valid SensorEvent header and an undersized payload") {
      SECTION("Should return nullopt") {
        AdcMessageBuilder builder(4);
        auto [header, _] = builder.BuildNoteOn(60, 100);
        std::array<std::uint8_t, 2> payload = {1, 60};

        auto result = MessageParser::Decode(header, payload);

        REQUIRE_FALSE(result.has_value());
      }
    }

    SECTION("When given a valid Command header") {
      SECTION("And the payload encodes a CalibStart with kAuto mode") {
        SECTION("Should reconstruct a CalibStart with kAuto") {
          MainBoardMessageBuilder builder;
          auto [header, _] = builder.BuildStartCalibration(4, CalibMode::kAuto);
          std::array<std::uint8_t, 2> payload = {0x03, 0x00};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          auto* cmd = std::get_if<Command>(&result.value());
          REQUIRE(cmd != nullptr);
          auto* calib = std::get_if<CalibStart>(cmd);
          REQUIRE(calib != nullptr);
          REQUIRE(calib->mode == CalibMode::kAuto);
        }
      }

      SECTION("And the payload encodes an AdcStart") {
        SECTION("Should reconstruct an AdcStart") {
          MainBoardMessageBuilder builder;
          auto [header, _] = builder.BuildStartCalibration(4, CalibMode::kAuto);
          std::array<std::uint8_t, 1> payload = {0x01};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          auto* cmd = std::get_if<Command>(&result.value());
          REQUIRE(cmd != nullptr);
          REQUIRE(std::get_if<AdcStart>(cmd) != nullptr);
        }
      }

      SECTION("And the payload encodes an unknown action code") {
        SECTION("Should return nullopt") {
          MainBoardMessageBuilder builder;
          auto [header, _] = builder.BuildStartCalibration(4, CalibMode::kAuto);
          std::array<std::uint8_t, 1> payload = {0xFF};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE_FALSE(result.has_value());
        }
      }
    }

    SECTION("When given a valid Heartbeat header") {
      SECTION("And the payload encodes a known DeviceState") {
        SECTION("Should reconstruct the Heartbeat with the correct state") {
          AdcMessageBuilder builder(2);
          auto [header, _] = builder.BuildHeartbeat(DeviceState::kRunning);
          std::array<std::uint8_t, 1> payload = {
              static_cast<std::uint8_t>(DeviceState::kRunning)};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          auto* hb = std::get_if<Heartbeat>(&result.value());
          REQUIRE(hb != nullptr);
          REQUIRE(hb->device_state == DeviceState::kRunning);
        }
      }

      SECTION("And the payload encodes an unknown device state") {
        SECTION("Should return nullopt") {
          AdcMessageBuilder builder(2);
          auto [header, _] = builder.BuildHeartbeat(DeviceState::kIdle);
          std::array<std::uint8_t, 1> payload = {0xFF};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE_FALSE(result.has_value());
        }
      }
    }
  }
}

#endif
