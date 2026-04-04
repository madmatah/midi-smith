#if defined(UNIT_TESTS)

#include "app/calibration/calibration_data_receiver.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "app/calibration/calibration_data_receiver_observer_requirements.hpp"
#include "app/calibration/merge_distance_values.hpp"
#include "app/config/calibration.hpp"
#include "app/config/config.hpp"
#include "app/config/sensors.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "calibration/sensor_calibration.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"
#include "protocol/messages.hpp"
#include "protocol/topology.hpp"

namespace {

using CalibrationDataReceiver = midismith::adc_board::app::calibration::CalibrationDataReceiver;
using CalibrationDataReceiverObserverRequirements =
    midismith::adc_board::app::calibration::CalibrationDataReceiverObserverRequirements;
using SensorCalibrationArray = CalibrationDataReceiverObserverRequirements::SensorCalibrationArray;
using SensorCalibration = midismith::calibration::SensorCalibration;
using CalibrationDataSegment = midismith::protocol::CalibrationDataSegment;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;

inline constexpr std::size_t kSensorCount =
    midismith::adc_board::app::config::sensors::kSensorCount;
inline constexpr std::size_t kTotalSegments =
    midismith::protocol_can::CanCalibrationSegmentPacker::ComputeTotalSegments(kSensorCount);

struct AckCall {
  std::uint8_t ack_index;
  DataSegmentAckStatus status;
};

class RecordingMessageSender final
    : public midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements {
 public:
  bool SendNoteOn(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }
  bool SendNoteOff(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }
  bool SendCalibrationLoadRequest() noexcept override {
    return true;
  }
  bool SendCalibrationDataSegment(
      std::uint8_t, std::uint8_t,
      const std::array<std::uint8_t, CalibrationDataSegment::kPayloadSizeBytes>&) noexcept
      override {
    return true;
  }

  bool SendDataSegmentAck(std::uint8_t ack_index, DataSegmentAckStatus status) noexcept override {
    ack_calls_.push_back({ack_index, status});
    return true;
  }

  [[nodiscard]] const std::vector<AckCall>& ack_calls() const noexcept {
    return ack_calls_;
  }

 private:
  std::vector<AckCall> ack_calls_;
};

class RecordingObserver final : public CalibrationDataReceiverObserverRequirements {
 public:
  void OnCalibrationDataReceived(const SensorCalibrationArray& data) noexcept override {
    received_data_ = data;
  }

  void OnCalibrationNoDataAvailable() noexcept override {
    no_data_available_called_ = true;
  }

  void OnCalibrationReceiveTimeout() noexcept override {
    timeout_called_ = true;
  }

  [[nodiscard]] const std::optional<SensorCalibrationArray>& received_data() const noexcept {
    return received_data_;
  }
  [[nodiscard]] bool no_data_available_called() const noexcept {
    return no_data_available_called_;
  }
  [[nodiscard]] bool timeout_called() const noexcept {
    return timeout_called_;
  }

 private:
  std::optional<SensorCalibrationArray> received_data_;
  bool no_data_available_called_ = false;
  bool timeout_called_ = false;
};

class RecordingTimer final : public midismith::os::TimerRequirements {
 public:
  bool Start(std::uint32_t period_ms) noexcept override {
    ++start_count_;
    last_period_ms_ = period_ms;
    return true;
  }

  bool Stop() noexcept override {
    ++stop_count_;
    return true;
  }

  [[nodiscard]] std::uint32_t start_count() const noexcept {
    return start_count_;
  }
  [[nodiscard]] std::uint32_t stop_count() const noexcept {
    return stop_count_;
  }
  [[nodiscard]] std::optional<std::uint32_t> last_period_ms() const noexcept {
    return last_period_ms_;
  }

 private:
  std::uint32_t start_count_ = 0;
  std::uint32_t stop_count_ = 0;
  std::optional<std::uint32_t> last_period_ms_;
};

SensorCalibrationArray MakeKnownCalibrationArray() noexcept {
  SensorCalibrationArray data{};
  for (std::size_t i = 0; i < kSensorCount; ++i) {
    data[i].rest_current_ma = static_cast<float>(i) * 1.0f;
    data[i].strike_current_ma = static_cast<float>(i) * 0.1f;
  }
  return data;
}

CalibrationDataSegment MakeSegment(const SensorCalibrationArray& calibration,
                                   std::uint8_t seq_index, std::uint8_t total_packets) noexcept {
  CalibrationDataSegment segment{};
  segment.seq_index = seq_index;
  segment.total_packets = total_packets;
  midismith::protocol_can::CanCalibrationSegmentPacker::PackSegment(
      calibration.data(), calibration.size(), seq_index, segment.payload.data());
  return segment;
}

void DeliverAllSegments(CalibrationDataReceiver& receiver,
                        const SensorCalibrationArray& calibration) noexcept {
  for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
    const auto segment = MakeSegment(calibration, static_cast<std::uint8_t>(seg),
                                     static_cast<std::uint8_t>(kTotalSegments));
    receiver.OnCalibrationDataSegment(segment, midismith::protocol::kMainBoardNodeId);
  }
}

}  // namespace

TEST_CASE("The CalibrationDataReceiver class") {
  RecordingMessageSender sender;
  RecordingObserver observer;
  RecordingTimer timer;
  CalibrationDataReceiver receiver(sender, observer, timer);

  const auto calibration = MakeKnownCalibrationArray();

  SECTION("The BeginReceiving() method") {
    SECTION("When called") {
      SECTION("Should start the timeout timer with kCalibrationLoadSegmentTimeoutMs") {
        receiver.BeginReceiving();

        REQUIRE(timer.start_count() == 1);
        REQUIRE(timer.last_period_ms() ==
                midismith::adc_board::app::config::kCalibrationLoadSegmentTimeoutMs);
      }
    }
  }

  SECTION("The OnCalibrationDataSegment() method") {
    receiver.BeginReceiving();

    SECTION("When receiving all segments in order from main-board") {
      SECTION("Should call OnCalibrationDataReceived with correctly reassembled data") {
        DeliverAllSegments(receiver, calibration);

        REQUIRE(observer.received_data().has_value());
        for (std::size_t i = 0; i < kSensorCount; ++i) {
          REQUIRE((*observer.received_data())[i].rest_current_ma == calibration[i].rest_current_ma);
          REQUIRE((*observer.received_data())[i].strike_current_ma ==
                  calibration[i].strike_current_ma);
        }
      }

      SECTION("Should send a DataSegmentAck with kOk for each segment") {
        DeliverAllSegments(receiver, calibration);

        REQUIRE(sender.ack_calls().size() == kTotalSegments);
        for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
          REQUIRE(sender.ack_calls()[seg].ack_index == static_cast<std::uint8_t>(seg));
          REQUIRE(sender.ack_calls()[seg].status == DataSegmentAckStatus::kOk);
        }
      }

      SECTION("Should stop the timeout timer after the last segment") {
        DeliverAllSegments(receiver, calibration);

        REQUIRE(timer.stop_count() == 1);
      }
    }

    SECTION("When total_packets is 0") {
      CalibrationDataSegment no_data_segment{};
      no_data_segment.seq_index = 0;
      no_data_segment.total_packets = 0;

      SECTION("Should stop the timer and call OnCalibrationNoDataAvailable") {
        receiver.OnCalibrationDataSegment(no_data_segment, midismith::protocol::kMainBoardNodeId);

        REQUIRE(observer.no_data_available_called());
        REQUIRE(timer.stop_count() == 1);
      }

      SECTION("Should not send an ACK") {
        receiver.OnCalibrationDataSegment(no_data_segment, midismith::protocol::kMainBoardNodeId);

        REQUIRE(sender.ack_calls().empty());
      }
    }

    SECTION("When only some segments have been received") {
      SECTION("Should not call OnCalibrationDataReceived") {
        const auto segment = MakeSegment(calibration, 0, static_cast<std::uint8_t>(kTotalSegments));
        receiver.OnCalibrationDataSegment(segment, midismith::protocol::kMainBoardNodeId);

        REQUIRE_FALSE(observer.received_data().has_value());
      }

      SECTION("Should restart the timeout timer on each segment") {
        const auto segment0 =
            MakeSegment(calibration, 0, static_cast<std::uint8_t>(kTotalSegments));
        const auto segment1 =
            MakeSegment(calibration, 1, static_cast<std::uint8_t>(kTotalSegments));

        receiver.OnCalibrationDataSegment(segment0, midismith::protocol::kMainBoardNodeId);
        receiver.OnCalibrationDataSegment(segment1, midismith::protocol::kMainBoardNodeId);

        // 1 from BeginReceiving + 2 from segments
        REQUIRE(timer.start_count() == 3);
      }
    }

    SECTION("When segments arrive out of order") {
      SECTION("Should complete and provide correct data when all segments arrive") {
        for (std::size_t seg = kTotalSegments; seg > 0; --seg) {
          const auto segment = MakeSegment(calibration, static_cast<std::uint8_t>(seg - 1),
                                           static_cast<std::uint8_t>(kTotalSegments));
          receiver.OnCalibrationDataSegment(segment, midismith::protocol::kMainBoardNodeId);
        }

        REQUIRE(observer.received_data().has_value());
        for (std::size_t i = 0; i < kSensorCount; ++i) {
          REQUIRE((*observer.received_data())[i].rest_current_ma == calibration[i].rest_current_ma);
          REQUIRE((*observer.received_data())[i].strike_current_ma ==
                  calibration[i].strike_current_ma);
        }
      }
    }
  }

  SECTION("The OnReceiveTimeout() method") {
    SECTION("When called with a valid receiver pointer") {
      SECTION("Should call OnCalibrationReceiveTimeout on the observer") {
        CalibrationDataReceiver::OnReceiveTimeout(&receiver);

        REQUIRE(observer.timeout_called());
      }
    }

    SECTION("When called with a null pointer") {
      SECTION("Should not crash") {
        CalibrationDataReceiver::OnReceiveTimeout(nullptr);
      }
    }
  }
}

TEST_CASE("The MergeDistanceValues function") {
  SECTION("The MergeDistanceValues() method") {
    SECTION("When called with calibrations having varying distance values") {
      SensorCalibrationArray calibrations{};
      for (std::size_t i = 0; i < kSensorCount; ++i) {
        calibrations[i].rest_current_ma = static_cast<float>(i) * 0.5f;
        calibrations[i].strike_current_ma = static_cast<float>(i) * 0.3f;
        calibrations[i].rest_distance_mm = static_cast<float>(i) * 2.0f;
        calibrations[i].strike_distance_mm = static_cast<float>(i) * 1.5f;
      }

      midismith::adc_board::app::calibration::MergeDistanceValues(calibrations);

      SECTION("Should overwrite all distance fields with config defaults") {
        for (std::size_t i = 0; i < kSensorCount; ++i) {
          REQUIRE(calibrations[i].rest_distance_mm ==
                  midismith::adc_board::app::config::kDefaultSensorCalibration.rest_distance_mm);
          REQUIRE(calibrations[i].strike_distance_mm ==
                  midismith::adc_board::app::config::kDefaultSensorCalibration.strike_distance_mm);
        }
      }

      SECTION("Should not modify current fields") {
        for (std::size_t i = 0; i < kSensorCount; ++i) {
          REQUIRE(calibrations[i].rest_current_ma == static_cast<float>(i) * 0.5f);
          REQUIRE(calibrations[i].strike_current_ma == static_cast<float>(i) * 0.3f);
        }
      }
    }
  }
}

#endif
