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
        REQUIRE(header.destination_node_id == 0);
        REQUIRE(msg.type == SensorEventType::kNoteOn);
        REQUIRE(msg.sensor_id == 60);
        REQUIRE(msg.velocity == 100);
      }
    }
  }
}

TEST_CASE("The MainBoardMessageBuilder class") {
  SECTION("The BuildStartCalibration() method") {
    SECTION("When called with a target ADC node and auto mode enabled") {
      SECTION("Should produce a kControl command targeting that node with auto mode parameter") {
        MainBoardMessageBuilder builder;
        auto [header, msg] = builder.BuildStartCalibration(5, true);

        REQUIRE(header.source_node_id == static_cast<std::uint8_t>(NodeRole::kMainBoard));
        REQUIRE(header.destination_node_id == 5);
        REQUIRE(msg.action_code == 0x03);
        REQUIRE(msg.parameter == 0x00);
      }
    }
  }
}

#endif
