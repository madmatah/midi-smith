#if defined(UNIT_TESTS)

#include "protocol/message_parser.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

using namespace midismith::protocol;

TEST_CASE("The MessageParser class", "[protocol]") {
  SECTION("The Decode() method") {
    SECTION("When given a RealTime message") {
      SECTION("With a SensorEvent type") {
        SECTION("And valid payload") {
          SECTION("Should reconstruct the SensorEvent and preserve routing") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kRealTime,
                                                      MessageType::kSensorEvent, 1, 0);
            std::array<std::uint8_t, 3> payload = {
                static_cast<std::uint8_t>(SensorEventType::kNoteOn), 60, 100};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE(result.has_value());
            auto* routing = std::get_if<UnicastTransportHeader>(&result->routing);
            REQUIRE(routing != nullptr);
            REQUIRE(routing->source_node_id == 1);
            auto* event = std::get_if<SensorEvent>(&result->content);
            REQUIRE(event != nullptr);
            REQUIRE(event->type == SensorEventType::kNoteOn);
            REQUIRE(event->sensor_id == 60);
          }
        }

        SECTION("And undersized payload") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kRealTime,
                                                      MessageType::kSensorEvent, 1, 0);
            std::array<std::uint8_t, 2> payload = {1, 60};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE_FALSE(result.has_value());
          }
        }

        SECTION("And invalid sensor event type") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kRealTime,
                                                      MessageType::kSensorEvent, 1, 0);
            std::array<std::uint8_t, 3> payload = {0xFF, 60, 100};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE_FALSE(result.has_value());
          }
        }
      }

      SECTION("With an unsupported type") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kRealTime,
                                                    MessageType::kHeartbeat, 1, 0);
          std::array<std::uint8_t, 1> payload = {0x00};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE_FALSE(result.has_value());
        }
      }
    }

    SECTION("When given a Control message") {
      SECTION("With a Command type") {
        SECTION("And empty payload") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0, 1);

            auto result = MessageParser::Decode(header, {});

            REQUIRE_FALSE(result.has_value());
          }
        }

        SECTION("And an AdcStart action") {
          SECTION("Should return AdcStart") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0, 1);
            std::array<std::uint8_t, 1> payload = {
                static_cast<std::uint8_t>(CommandAction::kAdcStart)};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE(result.has_value());
            auto* cmd = std::get_if<Command>(&result->content);
            REQUIRE(cmd != nullptr);
            REQUIRE(std::get_if<AdcStart>(cmd) != nullptr);
          }
        }

        SECTION("And an AdcStop action") {
          SECTION("Should return AdcStop") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0, 1);
            std::array<std::uint8_t, 1> payload = {
                static_cast<std::uint8_t>(CommandAction::kAdcStop)};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE(result.has_value());
            auto* cmd = std::get_if<Command>(&result->content);
            REQUIRE(cmd != nullptr);
            REQUIRE(std::get_if<AdcStop>(cmd) != nullptr);
          }
        }

        SECTION("And a CalibStart action") {
          SECTION("With valid payload") {
            SECTION("Should return CalibStart with correct mode") {
              auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                        MessageType::kCommand, 0, 1);
              std::array<std::uint8_t, 2> payload = {
                  static_cast<std::uint8_t>(CommandAction::kCalibStart),
                  static_cast<std::uint8_t>(CalibMode::kManual)};

              auto result = MessageParser::Decode(header, payload);

              REQUIRE(result.has_value());
              auto* cmd = std::get_if<Command>(&result->content);
              REQUIRE(cmd != nullptr);
              auto* calib = std::get_if<CalibStart>(cmd);
              REQUIRE(calib != nullptr);
              REQUIRE(calib->mode == CalibMode::kManual);
            }
          }

          SECTION("With undersized payload") {
            SECTION("Should return nullopt") {
              auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                        MessageType::kCommand, 0, 1);
              std::array<std::uint8_t, 1> payload = {
                  static_cast<std::uint8_t>(CommandAction::kCalibStart)};

              auto result = MessageParser::Decode(header, payload);

              REQUIRE_FALSE(result.has_value());
            }
          }

          SECTION("With invalid mode") {
            SECTION("Should return nullopt") {
              auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                        MessageType::kCommand, 0, 1);
              std::array<std::uint8_t, 2> payload = {
                  static_cast<std::uint8_t>(CommandAction::kCalibStart), 0xFF};

              auto result = MessageParser::Decode(header, payload);

              REQUIRE_FALSE(result.has_value());
            }
          }
        }

        SECTION("And a DumpRequest action") {
          SECTION("Should return DumpRequest") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0, 1);
            std::array<std::uint8_t, 1> payload = {
                static_cast<std::uint8_t>(CommandAction::kDumpRequest)};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE(result.has_value());
            auto* cmd = std::get_if<Command>(&result->content);
            REQUIRE(cmd != nullptr);
            REQUIRE(std::get_if<DumpRequest>(cmd) != nullptr);
          }
        }

        SECTION("And an unknown action") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0, 1);
            std::array<std::uint8_t, 1> payload = {0xFF};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE_FALSE(result.has_value());
          }
        }
      }

      SECTION("When given a broadcast Command") {
        SECTION("Should return the Command with broadcast routing") {
          auto header = BroadcastTransportHeader::Make(MessageCategory::kControl,
                                                      MessageType::kCommand, 0);
          std::array<std::uint8_t, 1> payload = {
              static_cast<std::uint8_t>(CommandAction::kAdcStart)};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          REQUIRE(std::get_if<BroadcastTransportHeader>(&result->routing) != nullptr);
          auto* cmd = std::get_if<Command>(&result->content);
          REQUIRE(cmd != nullptr);
          REQUIRE(std::get_if<AdcStart>(cmd) != nullptr);
        }
      }

      SECTION("With an unsupported type") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kControl,
                                                    MessageType::kHeartbeat, 0, 1);

          auto result = MessageParser::Decode(header, {});

          REQUIRE_FALSE(result.has_value());
        }
      }
    }

    SECTION("When given a System message") {
      SECTION("With a Heartbeat type") {
        SECTION("And valid payload") {
          SECTION("Should return Heartbeat") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kSystem,
                                                      MessageType::kHeartbeat, 1, 0);
            std::array<std::uint8_t, 1> payload = {
                static_cast<std::uint8_t>(DeviceState::kRunning)};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE(result.has_value());
            auto* hb = std::get_if<Heartbeat>(&result->content);
            REQUIRE(hb != nullptr);
            REQUIRE(hb->device_state == DeviceState::kRunning);
          }
        }

        SECTION("And empty payload") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kSystem,
                                                      MessageType::kHeartbeat, 1, 0);

            auto result = MessageParser::Decode(header, {});

            REQUIRE_FALSE(result.has_value());
          }
        }

        SECTION("And invalid state") {
          SECTION("Should return nullopt") {
            auto header = UnicastTransportHeader::Make(MessageCategory::kSystem,
                                                      MessageType::kHeartbeat, 1, 0);
            std::array<std::uint8_t, 1> payload = {0xFF};

            auto result = MessageParser::Decode(header, payload);

            REQUIRE_FALSE(result.has_value());
          }
        }
      }

      SECTION("With an unsupported type") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kSystem,
                                                    MessageType::kCommand, 1, 0);

          auto result = MessageParser::Decode(header, {});

          REQUIRE_FALSE(result.has_value());
        }
      }
    }

    SECTION("When given a BulkData message") {
      SECTION("With an unsupported type") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                    MessageType::kCommand, 1, 0);

          auto result = MessageParser::Decode(header, {});

          REQUIRE_FALSE(result.has_value());
        }
      }

      SECTION("With a kDataSegment type and valid payload") {
        SECTION("Should return CalibrationDataSegment with correct fields") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                    MessageType::kDataSegment, 1, 0);
          std::array<std::uint8_t, CalibrationDataSegment::kSerializedSizeBytes> payload{};
          payload[0] = 2;
          payload[1] = 8;
          payload[2] = 0x42;

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          auto* segment = std::get_if<CalibrationDataSegment>(&result->content);
          REQUIRE(segment != nullptr);
          REQUIRE(segment->seq_index == 2);
          REQUIRE(segment->total_packets == 8);
          REQUIRE(segment->payload[0] == 0x42);
        }
      }

      SECTION("With a kDataSegment type and undersized payload") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                    MessageType::kDataSegment, 1, 0);
          std::array<std::uint8_t, 1> payload{};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE_FALSE(result.has_value());
        }
      }

      SECTION("With a kDataSegmentAck type and valid payload") {
        SECTION("Should return DataSegmentAck with correct fields") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                    MessageType::kDataSegmentAck, 0, 1);
          std::array<std::uint8_t, 2> payload = {
              5, static_cast<std::uint8_t>(DataSegmentAckStatus::kCrcError)};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE(result.has_value());
          auto* ack = std::get_if<DataSegmentAck>(&result->content);
          REQUIRE(ack != nullptr);
          REQUIRE(ack->ack_index == 5);
          REQUIRE(ack->status == DataSegmentAckStatus::kCrcError);
        }
      }

      SECTION("With a kDataSegmentAck type and invalid status") {
        SECTION("Should return nullopt") {
          auto header = UnicastTransportHeader::Make(MessageCategory::kBulkData,
                                                    MessageType::kDataSegmentAck, 0, 1);
          std::array<std::uint8_t, 2> payload = {0, 0xFF};

          auto result = MessageParser::Decode(header, payload);

          REQUIRE_FALSE(result.has_value());
        }
      }
    }

    SECTION("When given an unknown category") {
      SECTION("Should return nullopt") {
        auto header = UnicastTransportHeader::Make(static_cast<MessageCategory>(0xFF),
                                                  MessageType::kCommand, 1, 0);

        auto result = MessageParser::Decode(header, {});

        REQUIRE_FALSE(result.has_value());
      }
    }
  }
}

#endif
