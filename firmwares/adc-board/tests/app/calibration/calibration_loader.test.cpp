#if defined(UNIT_TESTS)

#include "app/calibration/calibration_loader.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <optional>
#include <vector>

#include "app/calibration/calibration_apply_requirements.hpp"
#include "app/calibration/calibration_data_receiver.hpp"
#include "app/calibration/calibration_data_receiver_observer_requirements.hpp"
#include "app/config/config.hpp"
#include "app/config/sensor_linearization.hpp"
#include "app/config/sensors.hpp"
#include "app/messaging/adc_board_message_sender_requirements.hpp"
#include "app/supervisor/adc_supervisor_task.hpp"
#include "calibration/board_calibration_data.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"
#include "protocol/messages.hpp"
#include "protocol/topology.hpp"

namespace {

using Loader = midismith::adc_board::app::calibration::CalibrationLoader;
using State = Loader::State;
using SensorCalibration = midismith::calibration::SensorCalibration;
using CalibrationDataReceiver = midismith::adc_board::app::calibration::CalibrationDataReceiver;
using CalibrationDataSegment = midismith::protocol::CalibrationDataSegment;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;
using DeviceState = midismith::protocol::DeviceState;
using SupervisorEvent = midismith::adc_board::app::supervisor::AdcSupervisorTask::Event;
using InitComplete =
    midismith::adc_board::app::supervisor::AdcSupervisorTask::InitializationComplete;

inline constexpr std::size_t kSensorCount =
    midismith::adc_board::app::config::sensors::kSensorCount;
inline constexpr std::size_t kTotalSegments =
    midismith::protocol_can::CanCalibrationSegmentPacker::ComputeTotalSegments(kSensorCount);

using SensorCalibrationArray = midismith::calibration::BoardCalibrationData<kSensorCount>;

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
    ++calibration_load_request_count_;
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

  [[nodiscard]] std::uint32_t calibration_load_request_count() const noexcept {
    return calibration_load_request_count_;
  }
  [[nodiscard]] const std::vector<AckCall>& ack_calls() const noexcept {
    return ack_calls_;
  }

 private:
  std::uint32_t calibration_load_request_count_ = 0;
  std::vector<AckCall> ack_calls_;
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

class RecordingSupervisorQueue final : public midismith::os::QueueRequirements<SupervisorEvent> {
 public:
  bool Send(const SupervisorEvent& item, std::uint32_t) noexcept override {
    sent_events_.push_back(item);
    return true;
  }

  bool SendFromIsr(const SupervisorEvent&) noexcept override {
    return true;
  }

  bool Receive(SupervisorEvent&, std::uint32_t) noexcept override {
    return false;
  }

  [[nodiscard]] const std::vector<SupervisorEvent>& sent_events() const noexcept {
    return sent_events_;
  }

  [[nodiscard]] std::size_t initialization_complete_count() const noexcept {
    std::size_t count = 0;
    for (const auto& event : sent_events_) {
      if (std::holds_alternative<InitComplete>(event)) {
        ++count;
      }
    }
    return count;
  }

 private:
  std::vector<SupervisorEvent> sent_events_;
};

class RecordingCalibrationApply final
    : public midismith::adc_board::app::calibration::CalibrationApplyRequirements {
 public:
  void ApplyCalibration(const SensorCalibrationArray& data) noexcept override {
    apply_called = true;
    last_data = data;
  }
  void ApplySensorCalibration(std::uint8_t,
                              const midismith::calibration::SensorCalibration&) noexcept override {}

  bool apply_called = false;
  SensorCalibrationArray last_data{};
};

SensorCalibrationArray MakeKnownCalibrationArray() noexcept {
  SensorCalibrationArray data{};
  for (std::size_t i = 0; i < kSensorCount; ++i) {
    data[i].rest_current_ma = static_cast<float>(i) * 1.0f;
    data[i].strike_current_ma = static_cast<float>(i) * 0.1f;
    data[i].rest_distance_mm = 0.0f;
    data[i].strike_distance_mm = 0.0f;
  }
  return data;
}

CalibrationDataSegment PackSegment(const SensorCalibrationArray& source,
                                   std::size_t segment_index) noexcept {
  CalibrationDataSegment segment{};
  segment.seq_index = static_cast<std::uint8_t>(segment_index);
  segment.total_packets = static_cast<std::uint8_t>(kTotalSegments);
  midismith::protocol_can::CanCalibrationSegmentPacker::PackSegment(
      source.data(), kSensorCount, segment_index, segment.payload.data());
  return segment;
}

struct TestFixture {
  RecordingMessageSender sender;
  RecordingSupervisorQueue supervisor_queue;
  RecordingTimer retry_timer;
  RecordingTimer segment_timeout_timer;
  RecordingCalibrationApply calibration_apply;
  Loader loader{sender, supervisor_queue, retry_timer, calibration_apply};
  CalibrationDataReceiver receiver{sender, loader, segment_timeout_timer};

  TestFixture() noexcept {
    loader.SetReceiver(receiver);
  }

  void TransitionToReceiving() noexcept {
    loader.OnPeerHeartbeat(DeviceState::kReady);
  }

  void FeedAllSegments(const SensorCalibrationArray& data) noexcept {
    for (std::size_t i = 0; i < kTotalSegments; ++i) {
      auto segment = PackSegment(data, i);
      receiver.OnCalibrationDataSegment(segment, midismith::protocol::kMainBoardNodeId);
    }
  }
};

}  // namespace

TEST_CASE("The CalibrationLoader class") {
  TestFixture f;

  SECTION("The OnPeerHeartbeat() method") {
    SECTION("When in kWaitingForPeer state") {
      SECTION("Should send CalibrationLoadRequest") {
        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.sender.calibration_load_request_count() == 1);
      }

      SECTION("Should start the retry timer with kCalibrationLoadTimeoutMs") {
        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.retry_timer.start_count() == 1);
        REQUIRE(f.retry_timer.last_period_ms().value() ==
                midismith::adc_board::app::config::kCalibrationLoadTimeoutMs);
      }

      SECTION("Should start the segment timeout timer via BeginReceiving") {
        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.segment_timeout_timer.start_count() == 1);
      }

      SECTION("Should transition to kReceivingCalibration") {
        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.loader.state() == State::kReceivingCalibration);
      }
    }

    SECTION("When already in kComplete state") {
      f.TransitionToReceiving();
      f.loader.OnCalibrationNoDataAvailable();
      REQUIRE(f.loader.state() == State::kComplete);

      SECTION("Should not send any request") {
        auto count_before = f.sender.calibration_load_request_count();

        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.sender.calibration_load_request_count() == count_before);
      }
    }

    SECTION("When already in kReceivingCalibration state") {
      f.TransitionToReceiving();

      SECTION("Should not send another request") {
        auto count_before = f.sender.calibration_load_request_count();

        f.loader.OnPeerHeartbeat(DeviceState::kReady);

        REQUIRE(f.sender.calibration_load_request_count() == count_before);
      }
    }
  }

  SECTION("The OnPeerLost() method") {
    SECTION("When in kReceivingCalibration state") {
      f.TransitionToReceiving();

      SECTION("Should stop the retry timer") {
        auto stop_before = f.retry_timer.stop_count();

        f.loader.OnPeerLost();

        REQUIRE(f.retry_timer.stop_count() > stop_before);
      }

      SECTION("Should transition back to kWaitingForPeer") {
        f.loader.OnPeerLost();

        REQUIRE(f.loader.state() == State::kWaitingForPeer);
      }
    }

    SECTION("When in kWaitingForPeer state") {
      SECTION("Should remain in kWaitingForPeer") {
        f.loader.OnPeerLost();

        REQUIRE(f.loader.state() == State::kWaitingForPeer);
      }
    }

    SECTION("When in kComplete state") {
      f.TransitionToReceiving();
      f.loader.OnCalibrationNoDataAvailable();

      SECTION("Should remain in kComplete") {
        f.loader.OnPeerLost();

        REQUIRE(f.loader.state() == State::kComplete);
      }
    }
  }

  SECTION("The OnCalibrationDataReceived() method") {
    f.TransitionToReceiving();
    auto known_data = MakeKnownCalibrationArray();

    SECTION("When all segments are received") {
      f.FeedAllSegments(known_data);

      SECTION("Should stop the retry timer") {
        REQUIRE(f.retry_timer.stop_count() > 0);
      }

      SECTION("Should call ApplyCalibration with merged data") {
        REQUIRE(f.calibration_apply.apply_called);
        REQUIRE(f.calibration_apply.last_data[0].rest_distance_mm ==
                midismith::adc_board::app::config::kDefaultSensorCalibration.rest_distance_mm);
        REQUIRE(f.calibration_apply.last_data[0].strike_distance_mm ==
                midismith::adc_board::app::config::kDefaultSensorCalibration.strike_distance_mm);
        REQUIRE(f.calibration_apply.last_data[0].rest_current_ma == known_data[0].rest_current_ma);
      }

      SECTION("Should post InitializationComplete to supervisor queue") {
        REQUIRE(f.supervisor_queue.initialization_complete_count() == 1);
      }

      SECTION("Should transition to kComplete") {
        REQUIRE(f.loader.state() == State::kComplete);
      }
    }
  }

  SECTION("The OnCalibrationNoDataAvailable() method") {
    f.TransitionToReceiving();

    SECTION("When main-board reports no data") {
      CalibrationDataSegment no_data_segment{};
      no_data_segment.seq_index = 0;
      no_data_segment.total_packets = 0;
      f.receiver.OnCalibrationDataSegment(no_data_segment, midismith::protocol::kMainBoardNodeId);

      SECTION("Should stop the retry timer") {
        REQUIRE(f.retry_timer.stop_count() > 0);
      }

      SECTION("Should not call ApplyCalibration") {
        REQUIRE_FALSE(f.calibration_apply.apply_called);
      }

      SECTION("Should post InitializationComplete to supervisor queue") {
        REQUIRE(f.supervisor_queue.initialization_complete_count() == 1);
      }

      SECTION("Should transition to kComplete") {
        REQUIRE(f.loader.state() == State::kComplete);
      }
    }
  }

  SECTION("The OnCalibrationReceiveTimeout() method") {
    f.TransitionToReceiving();

    SECTION("When retry count is under the maximum") {
      f.loader.OnCalibrationReceiveTimeout();

      SECTION("Should resend CalibrationLoadRequest") {
        REQUIRE(f.sender.calibration_load_request_count() == 2);
      }

      SECTION("Should restart the segment timeout timer via BeginReceiving") {
        REQUIRE(f.segment_timeout_timer.start_count() >= 2);
      }

      SECTION("Should restart the retry timer") {
        REQUIRE(f.retry_timer.start_count() >= 2);
      }

      SECTION("Should remain in kReceivingCalibration") {
        REQUIRE(f.loader.state() == State::kReceivingCalibration);
      }
    }

    SECTION("When maximum retries are exceeded") {
      for (std::uint32_t i = 0; i < midismith::adc_board::app::config::kCalibrationLoadMaxRetries;
           ++i) {
        f.loader.OnCalibrationReceiveTimeout();
      }

      SECTION("Should post InitializationComplete to supervisor queue") {
        REQUIRE(f.supervisor_queue.initialization_complete_count() == 1);
      }

      SECTION("Should transition to kComplete") {
        REQUIRE(f.loader.state() == State::kComplete);
      }

      SECTION("Should not call ApplyCalibration") {
        REQUIRE_FALSE(f.calibration_apply.apply_called);
      }
    }
  }

  SECTION("The OnRequestRetryTimeout() static method") {
    SECTION("When called with null pointer") {
      Loader::OnRequestRetryTimeout(nullptr);
    }
  }

  SECTION("When peer is lost during calibration and then reconnects") {
    f.TransitionToReceiving();
    f.loader.OnPeerLost();
    REQUIRE(f.loader.state() == State::kWaitingForPeer);

    f.loader.OnPeerHeartbeat(DeviceState::kReady);
    REQUIRE(f.loader.state() == State::kReceivingCalibration);
    REQUIRE(f.sender.calibration_load_request_count() == 2);

    auto known_data = MakeKnownCalibrationArray();
    f.FeedAllSegments(known_data);

    REQUIRE(f.loader.state() == State::kComplete);
    REQUIRE(f.supervisor_queue.initialization_complete_count() == 1);
    REQUIRE(f.calibration_apply.apply_called);
  }
}

#endif
