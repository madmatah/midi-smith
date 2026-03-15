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

        REQUIRE(event.Serialize(buffer) == 3u);
        REQUIRE(buffer[0] == static_cast<std::uint8_t>(SensorEventType::kNoteOn));
        REQUIRE(buffer[1] == 60);
        REQUIRE(buffer[2] == 100);
      }
    }

    SECTION("When the output buffer is smaller than 3 bytes") {
      SECTION("Should return false") {
        SensorEvent event{SensorEventType::kNoteOn, 60, 100};
        std::array<std::uint8_t, 2> buffer{};

        REQUIRE_FALSE(event.Serialize(buffer).has_value());
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

        REQUIRE(hb.Serialize(buffer) == 1u);
        REQUIRE(buffer[0] == static_cast<std::uint8_t>(DeviceState::kCalibrating));
      }
    }

    SECTION("When the output buffer is empty") {
      SECTION("Should return false") {
        Heartbeat hb{DeviceState::kReady};
        std::array<std::uint8_t, 0> buffer{};

        REQUIRE_FALSE(hb.Serialize(buffer).has_value());
      }
    }
  }
}

TEST_CASE("The Command variant — Serialize()") {
  SECTION("AdcStart serializes to a single byte 0x01") {
    Command cmd = AdcStart{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == 1u);
    REQUIRE(buffer[0] == 0x01);
  }

  SECTION("AdcStop serializes to a single byte 0x02") {
    Command cmd = AdcStop{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == 1u);
    REQUIRE(buffer[0] == 0x02);
  }

  SECTION("CalibStart serializes to action byte 0x03 followed by the mode byte") {
    SECTION("With kAuto mode") {
      Command cmd = CalibStart{CalibMode::kAuto};
      std::array<std::uint8_t, 2> buffer{};

      REQUIRE(Serialize(cmd, buffer) == 2u);
      REQUIRE(buffer[0] == 0x03);
      REQUIRE(buffer[1] == static_cast<std::uint8_t>(CalibMode::kAuto));
    }

    SECTION("With kManual mode") {
      Command cmd = CalibStart{CalibMode::kManual};
      std::array<std::uint8_t, 2> buffer{};

      REQUIRE(Serialize(cmd, buffer) == 2u);
      REQUIRE(buffer[0] == 0x03);
      REQUIRE(buffer[1] == static_cast<std::uint8_t>(CalibMode::kManual));
    }

    SECTION("When the output buffer is smaller than 2 bytes") {
      SECTION("Should return false") {
        Command cmd = CalibStart{CalibMode::kAuto};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE_FALSE(Serialize(cmd, buffer).has_value());
      }
    }
  }

  SECTION("DumpRequest serializes to a single byte 0x04") {
    Command cmd = DumpRequest{};
    std::array<std::uint8_t, 1> buffer{};

    REQUIRE(Serialize(cmd, buffer) == 1u);
    REQUIRE(buffer[0] == 0x04);
  }
}

TEST_CASE("The CalibrationDataSegment struct") {
  SECTION("The Serialize() method") {
    SECTION("When the output buffer is large enough") {
      SECTION("Should write seq_index, total_packets, then the payload bytes") {
        CalibrationDataSegment segment{};
        segment.seq_index = 2;
        segment.total_packets = 8;
        segment.payload[0] = 0xAB;
        segment.payload[47] = 0xCD;
        std::array<std::uint8_t, CalibrationDataSegment::kSerializedSizeBytes> buffer{};

        REQUIRE(segment.Serialize(buffer) == CalibrationDataSegment::kSerializedSizeBytes);
        REQUIRE(buffer[0] == 2);
        REQUIRE(buffer[1] == 8);
        REQUIRE(buffer[2] == 0xAB);
        REQUIRE(buffer[49] == 0xCD);
      }
    }

    SECTION("When the output buffer is too small") {
      SECTION("Should return nullopt") {
        CalibrationDataSegment segment{};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE_FALSE(segment.Serialize(buffer).has_value());
      }
    }
  }

  SECTION("The Deserialize() method") {
    SECTION("With a valid buffer") {
      SECTION("Should reconstruct the segment with correct fields") {
        std::array<std::uint8_t, CalibrationDataSegment::kSerializedSizeBytes> buffer{};
        buffer[0] = 3;
        buffer[1] = 8;
        buffer[2] = 0x11;
        buffer[49] = 0x22;

        auto result = CalibrationDataSegment::Deserialize(buffer);

        REQUIRE(result.has_value());
        REQUIRE(result->seq_index == 3);
        REQUIRE(result->total_packets == 8);
        REQUIRE(result->payload[0] == 0x11);
        REQUIRE(result->payload[47] == 0x22);
      }
    }

    SECTION("With an undersized buffer") {
      SECTION("Should return nullopt") {
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE_FALSE(CalibrationDataSegment::Deserialize(buffer).has_value());
      }
    }
  }

  SECTION("Serialize then Deserialize round-trip") {
    SECTION("Should recover the original segment") {
      CalibrationDataSegment original{};
      original.seq_index = 5;
      original.total_packets = 8;
      original.payload[0] = 0x42;
      original.payload[47] = 0xFF;
      std::array<std::uint8_t, CalibrationDataSegment::kSerializedSizeBytes> buffer{};

      original.Serialize(buffer);
      auto recovered = CalibrationDataSegment::Deserialize(buffer);

      REQUIRE(recovered.has_value());
      REQUIRE(*recovered == original);
    }
  }
}

TEST_CASE("The DataSegmentAck struct") {
  SECTION("The Serialize() method") {
    SECTION("When the output buffer is large enough") {
      SECTION("Should write ack_index then the status byte") {
        DataSegmentAck ack{.ack_index = 4, .status = DataSegmentAckStatus::kCrcError};
        std::array<std::uint8_t, 2> buffer{};

        REQUIRE(ack.Serialize(buffer) == 2u);
        REQUIRE(buffer[0] == 4);
        REQUIRE(buffer[1] == static_cast<std::uint8_t>(DataSegmentAckStatus::kCrcError));
      }
    }

    SECTION("When the output buffer is too small") {
      SECTION("Should return nullopt") {
        DataSegmentAck ack{.ack_index = 0, .status = DataSegmentAckStatus::kOk};
        std::array<std::uint8_t, 1> buffer{};

        REQUIRE_FALSE(ack.Serialize(buffer).has_value());
      }
    }
  }

  SECTION("The Deserialize() method") {
    SECTION("With a valid buffer and kOk status") {
      SECTION("Should return the correct DataSegmentAck") {
        std::array<std::uint8_t, 2> buffer = {7,
                                              static_cast<std::uint8_t>(DataSegmentAckStatus::kOk)};

        auto result = DataSegmentAck::Deserialize(buffer);

        REQUIRE(result.has_value());
        REQUIRE(result->ack_index == 7);
        REQUIRE(result->status == DataSegmentAckStatus::kOk);
      }
    }

    SECTION("With a valid buffer and kFlashBusy status") {
      SECTION("Should return the correct DataSegmentAck") {
        std::array<std::uint8_t, 2> buffer = {
            0, static_cast<std::uint8_t>(DataSegmentAckStatus::kFlashBusy)};

        auto result = DataSegmentAck::Deserialize(buffer);

        REQUIRE(result.has_value());
        REQUIRE(result->status == DataSegmentAckStatus::kFlashBusy);
      }
    }

    SECTION("With an invalid status byte") {
      SECTION("Should return nullopt") {
        std::array<std::uint8_t, 2> buffer = {0, 0xFF};

        REQUIRE_FALSE(DataSegmentAck::Deserialize(buffer).has_value());
      }
    }

    SECTION("With an undersized buffer") {
      SECTION("Should return nullopt") {
        std::array<std::uint8_t, 1> buffer = {0};

        REQUIRE_FALSE(DataSegmentAck::Deserialize(buffer).has_value());
      }
    }
  }

  SECTION("Serialize then Deserialize round-trip") {
    SECTION("Should recover the original ack") {
      DataSegmentAck original{.ack_index = 6, .status = DataSegmentAckStatus::kFlashBusy};
      std::array<std::uint8_t, 2> buffer{};

      original.Serialize(buffer);
      auto recovered = DataSegmentAck::Deserialize(buffer);

      REQUIRE(recovered.has_value());
      REQUIRE(*recovered == original);
    }
  }
}

#endif
