#if defined(UNIT_TESTS)

#include "protocol-can/can_mapper.hpp"

#include <catch2/catch_test_macros.hpp>

#include "protocol/builders.hpp"
#include "protocol/topology.hpp"
#include "protocol/transport_header.hpp"

using namespace midismith::protocol;
using midismith::protocol_can::CanIdentifierMapper;

TEST_CASE("The CanIdentifierMapper class") {
  SECTION("The EncodeId() method") {
    SECTION("When the header category is kRealTime") {
      SECTION("Should place the source node ID in the 0x01x identifier range") {
        AdcMessageBuilder builder(3);
        auto [header, note_event] = builder.BuildNoteOn(60, 100);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x013);
      }

      SECTION("Should produce a distinct identifier for each ADC node") {
        for (std::uint8_t node_id = 1; node_id <= 8; ++node_id) {
          AdcMessageBuilder builder(node_id);
          auto [header, note_event] = builder.BuildNoteOn(60, 100);

          REQUIRE(CanIdentifierMapper::EncodeId(header) == (0x010 | node_id));
        }
      }
    }

    SECTION("When the header category is kControl broadcast") {
      SECTION("Should encode to exactly 0x100") {
        auto header = BroadcastTransportHeader::Make(MessageCategory::kControl,
                                                     MessageType::kCommand, kMainBoardNodeId);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x100);
      }
    }

    SECTION("When the header category is kControl with a specific ADC destination") {
      SECTION("Should place the destination node ID in the 0x11x identifier range") {
        MainBoardMessageBuilder builder;
        auto [header, cmd] = builder.BuildStartCalibration(5, CalibMode::kAuto);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x115);
      }

      SECTION("Should produce a distinct identifier for each ADC node") {
        for (std::uint8_t node_id = 1; node_id <= 8; ++node_id) {
          MainBoardMessageBuilder builder;
          auto [header, cmd] = builder.BuildStartCalibration(node_id, CalibMode::kManual);

          REQUIRE(CanIdentifierMapper::EncodeId(header) == (0x110 | node_id));
        }
      }
    }

    SECTION("When the header category is kBulkData in DUMP direction") {
      SECTION("Should place the ADC source node ID in the 0x21x identifier range") {
        auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                  MessageType::kDataSegment, 4, 0);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x214);
      }
    }

    SECTION("When the header category is kBulkData in LOAD direction") {
      SECTION("Should place the ADC destination node ID in the 0x21x identifier range") {
        auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                  MessageType::kDataSegment, 0, 6);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x216);
      }
    }

    SECTION("When the header category is kSystem") {
      SECTION("Should place the source node ID in the 0x71x identifier range") {
        AdcMessageBuilder builder(2);
        auto [header, heartbeat] = builder.BuildHeartbeat(DeviceState::kRunning);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x712);
      }

      SECTION("Main board heartbeat should encode to exactly 0x710") {
        MainBoardMessageBuilder builder;
        auto [header, heartbeat] = builder.BuildHeartbeat(DeviceState::kRunning);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x710);
      }
    }
  }

  SECTION("The DecodeId() method") {
    SECTION("When given an identifier in the 0x01x range") {
      SECTION("Should reconstruct a UnicastTransportHeader for kRealTime kSensorEvent") {
        auto result = CanIdentifierMapper::DecodeId(0x013);

        REQUIRE(result.has_value());
        auto* header = std::get_if<UnicastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kRealTime);
        REQUIRE(header->type == MessageType::kSensorEvent);
        REQUIRE(header->source_node_id == 3);
        REQUIRE(header->destination_node_id == kMainBoardNodeId);
      }
    }

    SECTION("When given exactly 0x100") {
      SECTION("Should reconstruct a BroadcastTransportHeader for kControl kCommand") {
        auto result = CanIdentifierMapper::DecodeId(0x100);

        REQUIRE(result.has_value());
        auto* header = std::get_if<BroadcastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kControl);
        REQUIRE(header->type == MessageType::kCommand);
        REQUIRE(header->source_node_id == kMainBoardNodeId);
      }
    }

    SECTION("When given an identifier in the 0x11x range") {
      SECTION("Should reconstruct a UnicastTransportHeader for kControl kCommand") {
        auto result = CanIdentifierMapper::DecodeId(0x115);

        REQUIRE(result.has_value());
        auto* header = std::get_if<UnicastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kControl);
        REQUIRE(header->type == MessageType::kCommand);
        REQUIRE(header->source_node_id == kMainBoardNodeId);
        REQUIRE(header->destination_node_id == 5);
      }
    }

    SECTION("When given an identifier in the 0x21x range") {
      SECTION("Should reconstruct a UnicastTransportHeader for kBulkData kDataSegment") {
        auto result = CanIdentifierMapper::DecodeId(0x214);

        REQUIRE(result.has_value());
        auto* header = std::get_if<UnicastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kBulkData);
        REQUIRE(header->type == MessageType::kDataSegment);
        REQUIRE(header->source_node_id == 4);
        REQUIRE(header->destination_node_id == kMainBoardNodeId);
      }
    }

    SECTION("When given 0x710 (main board heartbeat)") {
      SECTION("Should reconstruct a BroadcastTransportHeader for kSystem kHeartbeat") {
        auto result = CanIdentifierMapper::DecodeId(0x710);

        REQUIRE(result.has_value());
        auto* header = std::get_if<BroadcastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kSystem);
        REQUIRE(header->type == MessageType::kHeartbeat);
        REQUIRE(header->source_node_id == kMainBoardNodeId);
      }
    }

    SECTION("When given an identifier in the 0x71x range (ADC heartbeat)") {
      SECTION("Should reconstruct a UnicastTransportHeader for kSystem kHeartbeat") {
        auto result = CanIdentifierMapper::DecodeId(0x712);

        REQUIRE(result.has_value());
        auto* header = std::get_if<UnicastTransportHeader>(&*result);
        REQUIRE(header != nullptr);
        REQUIRE(header->category == MessageCategory::kSystem);
        REQUIRE(header->type == MessageType::kHeartbeat);
        REQUIRE(header->source_node_id == 2);
        REQUIRE(header->destination_node_id == kMainBoardNodeId);
      }
    }

    SECTION("When given an identifier with bits beyond the 11-bit standard range") {
      SECTION("Should return nullopt") {
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x800).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0xFFF).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0xFFFF).has_value());
      }
    }

    SECTION("When given an identifier with an unknown function code") {
      SECTION("Should return nullopt") {
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x000).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x020).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x220).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x300).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x7FF).has_value());
      }
    }

    SECTION("When given a 0x10x identifier with a non-zero node ID") {
      SECTION("Should return nullopt") {
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x101).has_value());
        REQUIRE_FALSE(CanIdentifierMapper::DecodeId(0x10F).has_value());
      }
    }
  }

  SECTION("The EncodeId() and DecodeId() methods used together") {
    SECTION("When encoding and then decoding a kRealTime header") {
      SECTION("Should recover the original header without loss") {
        AdcMessageBuilder builder(7);
        auto [header, note] = builder.BuildNoteOn(60, 100);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<UnicastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }

    SECTION("When encoding and then decoding a kControl broadcast header") {
      SECTION("Should recover the original header without loss") {
        auto header = BroadcastTransportHeader::Make(MessageCategory::kControl,
                                                     MessageType::kCommand, kMainBoardNodeId);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<BroadcastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }

    SECTION("When encoding and then decoding a kControl unicast header") {
      SECTION("Should recover the original header without loss") {
        MainBoardMessageBuilder builder;
        auto [header, cmd] = builder.BuildStartCalibration(3, CalibMode::kManual);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<UnicastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }

    SECTION("When encoding and then decoding a kBulkData DUMP header") {
      SECTION("Should recover the original header without loss") {
        auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                  MessageType::kDataSegment, 4, 0);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<UnicastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }

    SECTION("When encoding and then decoding a kSystem ADC heartbeat header") {
      SECTION("Should recover the original header without loss") {
        AdcMessageBuilder builder(1);
        auto [header, hb] = builder.BuildHeartbeat(DeviceState::kIdle);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<UnicastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }

    SECTION("When encoding and then decoding a kSystem main board heartbeat header") {
      SECTION("Should recover the original header without loss") {
        MainBoardMessageBuilder builder;
        auto [header, hb] = builder.BuildHeartbeat(DeviceState::kRunning);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        auto* decoded_header = std::get_if<BroadcastTransportHeader>(&*decoded);
        REQUIRE(decoded_header != nullptr);
        REQUIRE(*decoded_header == header);
      }
    }
  }
}

#endif
