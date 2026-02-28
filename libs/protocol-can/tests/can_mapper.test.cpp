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

    SECTION("When the header category is kControl and destination is the main board") {
      SECTION("Should encode to exactly 0x100") {
        auto header = TransportHeader::ReconstructFromTransport(MessageCategory::kControl,
                                                                MessageType::kCommand, 0, 0);

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
        auto header = TransportHeader::ReconstructFromTransport(MessageCategory::kBulkData,
                                                                MessageType::kDataSegment, 4, 0);

        REQUIRE(CanIdentifierMapper::EncodeId(header) == 0x214);
      }
    }

    SECTION("When the header category is kBulkData in LOAD direction") {
      SECTION("Should place the ADC destination node ID in the 0x21x identifier range") {
        auto header = TransportHeader::ReconstructFromTransport(MessageCategory::kBulkData,
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
    }
  }

  SECTION("The DecodeId() method") {
    SECTION("When given an identifier in the 0x01x range") {
      SECTION("Should reconstruct a kRealTime kSensorEvent header with node ID as source") {
        auto result = CanIdentifierMapper::DecodeId(0x013);

        REQUIRE(result.has_value());
        REQUIRE(result->category == MessageCategory::kRealTime);
        REQUIRE(result->type == MessageType::kSensorEvent);
        REQUIRE(result->source_node_id == 3);
        REQUIRE(result->destination_node_id == 0);
      }
    }

    SECTION("When given exactly 0x100") {
      SECTION("Should reconstruct a kControl kCommand broadcast header") {
        auto result = CanIdentifierMapper::DecodeId(0x100);

        REQUIRE(result.has_value());
        REQUIRE(result->category == MessageCategory::kControl);
        REQUIRE(result->type == MessageType::kCommand);
        REQUIRE(result->source_node_id == 0);
        REQUIRE(result->destination_node_id == 0);
      }
    }

    SECTION("When given an identifier in the 0x11x range") {
      SECTION("Should reconstruct a kControl kCommand unicast header with node ID as destination") {
        auto result = CanIdentifierMapper::DecodeId(0x115);

        REQUIRE(result.has_value());
        REQUIRE(result->category == MessageCategory::kControl);
        REQUIRE(result->type == MessageType::kCommand);
        REQUIRE(result->source_node_id == 0);
        REQUIRE(result->destination_node_id == 5);
      }
    }

    SECTION("When given an identifier in the 0x21x range") {
      SECTION("Should reconstruct a kBulkData kDataSegment header with node ID as source") {
        auto result = CanIdentifierMapper::DecodeId(0x214);

        REQUIRE(result.has_value());
        REQUIRE(result->category == MessageCategory::kBulkData);
        REQUIRE(result->type == MessageType::kDataSegment);
        REQUIRE(result->source_node_id == 4);
        REQUIRE(result->destination_node_id == 0);
      }
    }

    SECTION("When given an identifier in the 0x71x range") {
      SECTION("Should reconstruct a kSystem kHeartbeat header with node ID as source") {
        auto result = CanIdentifierMapper::DecodeId(0x712);

        REQUIRE(result.has_value());
        REQUIRE(result->category == MessageCategory::kSystem);
        REQUIRE(result->type == MessageType::kHeartbeat);
        REQUIRE(result->source_node_id == 2);
        REQUIRE(result->destination_node_id == 0);
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
        REQUIRE(*decoded == header);
      }
    }

    SECTION("When encoding and then decoding a kControl broadcast header") {
      SECTION("Should recover the original header without loss") {
        auto header = TransportHeader::ReconstructFromTransport(MessageCategory::kControl,
                                                                MessageType::kCommand, 0, 0);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == header);
      }
    }

    SECTION("When encoding and then decoding a kControl unicast header") {
      SECTION("Should recover the original header without loss") {
        MainBoardMessageBuilder builder;
        auto [header, cmd] = builder.BuildStartCalibration(3, CalibMode::kManual);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == header);
      }
    }

    SECTION("When encoding and then decoding a kBulkData DUMP header") {
      SECTION("Should recover the original header without loss") {
        auto header = TransportHeader::ReconstructFromTransport(MessageCategory::kBulkData,
                                                                MessageType::kDataSegment, 4, 0);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == header);
      }
    }

    SECTION("When encoding and then decoding a kSystem header") {
      SECTION("Should recover the original header without loss") {
        AdcMessageBuilder builder(1);
        auto [header, hb] = builder.BuildHeartbeat(DeviceState::kIdle);

        auto decoded = CanIdentifierMapper::DecodeId(CanIdentifierMapper::EncodeId(header));

        REQUIRE(decoded.has_value());
        REQUIRE(*decoded == header);
      }
    }
  }
}

#endif
