#if defined(UNIT_TESTS)

#include "app/calibration/calibration_bulk_data_receiver.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <optional>
#include <vector>

#include "app/calibration/calibration_bulk_data_receiver_observer_requirements.hpp"
#include "app/config/config.hpp"
#include "app/messaging/main_board_message_sender_requirements.hpp"
#include "domain/config/main_board_config.hpp"
#include "protocol/messages.hpp"
#include "sensor-linearization/sensor_calibration.hpp"

namespace {

using CalibrationBulkDataReceiver =
    midismith::main_board::app::calibration::CalibrationBulkDataReceiver;
using CalibrationBulkDataReceiverObserverRequirements =
    midismith::main_board::app::calibration::CalibrationBulkDataReceiverObserverRequirements;
using SensorCalibrationArray =
    CalibrationBulkDataReceiverObserverRequirements::SensorCalibrationArray;
using SensorCalibration = midismith::sensor_linearization::SensorCalibration;
using CalibrationDataSegment = midismith::protocol::CalibrationDataSegment;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;

inline constexpr std::size_t kSensorsPerBoard =
    midismith::main_board::domain::config::kSensorsPerBoard;
inline constexpr std::size_t kSensorsPerSegment = CalibrationDataSegment::kSensorsPerSegment;
inline constexpr std::size_t kSensorCalibrationSizeBytes =
    CalibrationDataSegment::kSensorCalibrationSizeBytes;
inline constexpr std::size_t kTotalSegments =
    (kSensorsPerBoard + kSensorsPerSegment - 1) / kSensorsPerSegment;

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  struct AckCall {
    std::uint8_t target_node_id;
    std::uint8_t ack_index;
    DataSegmentAckStatus status;
  };

  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }
  bool SendStartAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStopAdc(std::uint8_t) noexcept override {
    return true;
  }
  bool SendStartCalibration(std::uint8_t, midismith::protocol::CalibMode) noexcept override {
    return true;
  }
  bool SendDumpRequest(std::uint8_t) noexcept override {
    return true;
  }

  bool SendCalibrationAck(std::uint8_t target_node_id, std::uint8_t ack_index,
                          DataSegmentAckStatus status) noexcept override {
    ack_calls_.push_back({target_node_id, ack_index, status});
    return true;
  }

  [[nodiscard]] const std::vector<AckCall>& ack_calls() const noexcept {
    return ack_calls_;
  }

 private:
  std::vector<AckCall> ack_calls_;
};

class RecordingObserver final : public CalibrationBulkDataReceiverObserverRequirements {
 public:
  void OnDataReceived(std::uint8_t board_id, const SensorCalibrationArray& data) noexcept override {
    received_board_id_ = board_id;
    received_data_ = data;
  }

  void OnReceiveTimeout(std::uint8_t board_id) noexcept override {
    timeout_board_id_ = board_id;
  }

  [[nodiscard]] const std::optional<std::uint8_t>& received_board_id() const noexcept {
    return received_board_id_;
  }
  [[nodiscard]] const std::optional<SensorCalibrationArray>& received_data() const noexcept {
    return received_data_;
  }
  [[nodiscard]] const std::optional<std::uint8_t>& timeout_board_id() const noexcept {
    return timeout_board_id_;
  }

 private:
  std::optional<std::uint8_t> received_board_id_;
  std::optional<SensorCalibrationArray> received_data_;
  std::optional<std::uint8_t> timeout_board_id_;
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
  for (std::size_t i = 0; i < kSensorsPerBoard; ++i) {
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

  for (std::size_t slot = 0; slot < kSensorsPerSegment; ++slot) {
    const std::size_t sensor_index = seq_index * kSensorsPerSegment + slot;
    if (sensor_index < kSensorsPerBoard) {
      std::memcpy(segment.payload.data() + slot * kSensorCalibrationSizeBytes,
                  &calibration[sensor_index], sizeof(SensorCalibration));
    }
  }

  return segment;
}

}  // namespace

TEST_CASE("CalibrationBulkDataReceiver") {
  RecordingMessageSender sender;
  RecordingObserver observer;
  RecordingTimer timer;
  CalibrationBulkDataReceiver receiver(sender, observer, timer);

  const std::uint8_t kBoardId = 2;
  const auto calibration = MakeKnownCalibrationArray();

  SECTION("BeginReceiving starts the timeout timer with correct period") {
    receiver.BeginReceiving(kBoardId);

    REQUIRE(timer.start_count() == 1);
    REQUIRE(timer.last_period_ms() ==
            midismith::main_board::app::config::kCalibrationReceiveTimeoutMs);
  }

  SECTION("Receiving all segments triggers OnDataReceived with correct data") {
    receiver.BeginReceiving(kBoardId);

    for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
      const auto segment = MakeSegment(calibration, static_cast<std::uint8_t>(seg),
                                       static_cast<std::uint8_t>(kTotalSegments));
      receiver.OnCalibrationDataSegment(segment, kBoardId);
    }

    REQUIRE(observer.received_board_id() == kBoardId);
    REQUIRE(observer.received_data().has_value());
    for (std::size_t i = 0; i < kSensorsPerBoard; ++i) {
      REQUIRE((*observer.received_data())[i].rest_current_ma == calibration[i].rest_current_ma);
      REQUIRE((*observer.received_data())[i].strike_current_ma == calibration[i].strike_current_ma);
    }
  }

  SECTION("A CalibrationAck is sent after each received segment") {
    receiver.BeginReceiving(kBoardId);

    for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
      const auto segment = MakeSegment(calibration, static_cast<std::uint8_t>(seg),
                                       static_cast<std::uint8_t>(kTotalSegments));
      receiver.OnCalibrationDataSegment(segment, kBoardId);
    }

    REQUIRE(sender.ack_calls().size() == kTotalSegments);
    for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
      REQUIRE(sender.ack_calls()[seg].target_node_id == kBoardId);
      REQUIRE(sender.ack_calls()[seg].ack_index == static_cast<std::uint8_t>(seg));
      REQUIRE(sender.ack_calls()[seg].status == DataSegmentAckStatus::kOk);
    }
  }

  SECTION("Timer is stopped after all segments received") {
    receiver.BeginReceiving(kBoardId);

    for (std::size_t seg = 0; seg < kTotalSegments; ++seg) {
      const auto segment = MakeSegment(calibration, static_cast<std::uint8_t>(seg),
                                       static_cast<std::uint8_t>(kTotalSegments));
      receiver.OnCalibrationDataSegment(segment, kBoardId);
    }

    REQUIRE(timer.stop_count() == 1);
  }

  SECTION("Segment from wrong board is ignored") {
    receiver.BeginReceiving(kBoardId);

    const auto segment = MakeSegment(calibration, 0, static_cast<std::uint8_t>(kTotalSegments));
    receiver.OnCalibrationDataSegment(segment, kBoardId + 1);

    REQUIRE(sender.ack_calls().empty());
    REQUIRE_FALSE(observer.received_board_id().has_value());
  }

  SECTION("Timeout callback triggers OnReceiveTimeout with correct board_id") {
    receiver.BeginReceiving(kBoardId);

    CalibrationBulkDataReceiver::OnReceiveTimeout(&receiver);

    REQUIRE(observer.timeout_board_id() == kBoardId);
  }

  SECTION("OnDataReceived is not called when only some segments received") {
    receiver.BeginReceiving(kBoardId);

    const auto segment = MakeSegment(calibration, 0, static_cast<std::uint8_t>(kTotalSegments));
    receiver.OnCalibrationDataSegment(segment, kBoardId);

    REQUIRE_FALSE(observer.received_board_id().has_value());
  }
}

#endif
