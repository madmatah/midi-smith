#if defined(UNIT_TESTS)

#include "protocol/builders.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace midismith::protocol;

TEST_CASE("The AdcMessageBuilder class") {
  SECTION("The BuildNoteOn() method") {
    SECTION("When called for a specific ADC node") {
      SECTION("Should produce a kRealTime header with that ADC node as source") {
        AdcMessageBuilder builder(4);
        auto [header, msg] = builder.BuildNoteOn(60, 100);

        REQUIRE(header.category == MessageCategory::kRealTime);
        REQUIRE(header.type == MessageType::kSensorEvent);
        REQUIRE(header.source_node_id == 4);
        REQUIRE(header.destination_node_id == kMainBoardNodeId);
        REQUIRE(msg.type == SensorEventType::kNoteOn);
        REQUIRE(msg.sensor_id == 60);
        REQUIRE(msg.velocity == 100);
      }
    }
  }

  SECTION("The BuildHeartbeat() method") {
    SECTION("Should carry the provided DeviceState") {
      AdcMessageBuilder builder(2);
      auto [header, msg] = builder.BuildHeartbeat(DeviceState::kRunning);

      REQUIRE(header.category == MessageCategory::kSystem);
      REQUIRE(header.type == MessageType::kHeartbeat);
      REQUIRE(msg.device_state == DeviceState::kRunning);
    }
  }
}

TEST_CASE("The MainBoardMessageBuilder class") {
  SECTION("The BuildStartCalibration() method") {
    SECTION("When called with kAuto mode") {
      SECTION("Should produce a kControl CalibStart command with kAuto mode") {
        MainBoardMessageBuilder builder;
        auto [header, cmd] = builder.BuildStartCalibration(5, CalibMode::kAuto);

        REQUIRE(header.source_node_id == kMainBoardNodeId);
        REQUIRE(header.destination_node_id == 5);
        auto* calib = std::get_if<CalibStart>(&cmd);
        REQUIRE(calib != nullptr);
        REQUIRE(calib->mode == CalibMode::kAuto);
      }
    }

    SECTION("When called with kManual mode") {
      SECTION("Should produce a kControl CalibStart command with kManual mode") {
        MainBoardMessageBuilder builder;
        auto [header, cmd] = builder.BuildStartCalibration(3, CalibMode::kManual);

        REQUIRE(header.destination_node_id == 3);
        auto* calib = std::get_if<CalibStart>(&cmd);
        REQUIRE(calib != nullptr);
        REQUIRE(calib->mode == CalibMode::kManual);
      }
    }
  }
}

#endif
