#if defined(UNIT_TESTS)

#include "app/calibration/calibration_task.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "protocol/messages.hpp"

namespace {

using CalibrationTask = midismith::adc_board::app::calibration::CalibrationTask;
using Event = CalibrationTask::Event;
using StartTransfer = CalibrationTask::StartTransfer;
using AckReceived = CalibrationTask::AckReceived;
using TimeoutTick = CalibrationTask::TimeoutTick;
using CalibrationArray = CalibrationTask::CalibrationArray;
using SegmentPayload = CalibrationTask::SegmentPayload;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;

class RecordingMessageSender final
    : public midismith::adc_board::app::messaging::AdcBoardMessageSenderRequirements {
 public:
  struct SegmentCall {
    std::uint8_t seq_index;
    std::uint8_t total_segments;
    SegmentPayload payload;
  };

  bool SendNoteOn(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }
  bool SendNoteOff(std::uint8_t, std::uint8_t) noexcept override {
    return true;
  }
  bool SendHeartbeat(midismith::protocol::DeviceState) noexcept override {
    return true;
  }

  bool SendCalibrationDataSegment(
      std::uint8_t seq_index, std::uint8_t total_segments,
      const std::array<std::uint8_t,
                       midismith::protocol::CalibrationDataSegment::kPayloadSizeBytes>&
          payload) noexcept override {
    segment_calls_.push_back({seq_index, total_segments, payload});
    return true;
  }

  [[nodiscard]] const std::vector<SegmentCall>& segment_calls() const noexcept {
    return segment_calls_;
  }

  [[nodiscard]] std::size_t segment_call_count() const noexcept {
    return segment_calls_.size();
  }

 private:
  std::vector<SegmentCall> segment_calls_;
};

class StubEventQueue final : public midismith::os::QueueRequirements<Event> {
 public:
  void Push(Event event) {
    pending_events_.push(std::move(event));
  }

  bool Send(const Event& item, std::uint32_t) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool SendFromIsr(const Event& item) noexcept override {
    pending_events_.push(item);
    return true;
  }

  bool Receive(Event& item, std::uint32_t) noexcept override {
    if (pending_events_.empty()) {
      return false;
    }
    item = pending_events_.front();
    pending_events_.pop();
    return true;
  }

 private:
  std::queue<Event> pending_events_;
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

midismith::protocol::DataSegmentAck MakeAck(DataSegmentAckStatus status) {
  midismith::protocol::DataSegmentAck ack{};
  ack.status = status;
  return ack;
}

CalibrationArray MakeCalibrationArray() {
  CalibrationArray data{};
  for (std::size_t i = 0; i < data.size(); ++i) {
    data[i].rest_current_ma = static_cast<float>(i) * 1.0f;
    data[i].strike_current_ma = static_cast<float>(i) * 2.0f;
    data[i].rest_distance_mm = static_cast<float>(i) * 3.0f;
    data[i].strike_distance_mm = static_cast<float>(i) * 4.0f;
  }
  return data;
}

}  // namespace

TEST_CASE("The CalibrationTask class") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  RecordingTimer timer;
  CalibrationTask task(sender, queue, timer);

  SECTION("The OnAckTimeout() static method") {
    SECTION("When called with a valid queue pointer") {
      SECTION("Should post a TimeoutTick event to the queue") {
        queue.Push(StartTransfer{MakeCalibrationArray()});

        CalibrationTask::OnAckTimeout(&queue);

        task.Run();

        REQUIRE(sender.segment_call_count() >= 2);
      }
    }

    SECTION("When called with a null pointer") {
      SECTION("Should not crash") {
        CalibrationTask::OnAckTimeout(nullptr);
        task.Run();
        REQUIRE(sender.segment_call_count() == 0);
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When a StartTransfer event is received") {
      SECTION("Should send segment 0 with seq_index=0 and total=kTotalSegments") {
        queue.Push(StartTransfer{MakeCalibrationArray()});

        task.Run();

        REQUIRE(sender.segment_call_count() == 1);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[0].total_segments == CalibrationTask::kTotalSegments);
      }

      SECTION("Should start the ack timer") {
        queue.Push(StartTransfer{MakeCalibrationArray()});

        task.Run();

        REQUIRE(timer.start_count() == 1);
      }
    }

    SECTION("When all kTotalSegments AckReceived(kOk) events follow a StartTransfer") {
      SECTION("Should send exactly kTotalSegments segments with sequential seq_index values") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        for (std::size_t i = 0; i < CalibrationTask::kTotalSegments; ++i) {
          queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kOk)});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationTask::kTotalSegments);
        for (std::size_t i = 0; i < CalibrationTask::kTotalSegments; ++i) {
          REQUIRE(sender.segment_calls()[i].seq_index == static_cast<std::uint8_t>(i));
        }
      }

      SECTION("Should stop the timer on each kOk ack") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        for (std::size_t i = 0; i < CalibrationTask::kTotalSegments; ++i) {
          queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kOk)});
        }

        task.Run();

        REQUIRE(timer.stop_count() == CalibrationTask::kTotalSegments);
      }
    }

    SECTION("When an AckReceived(kCrcError) follows a StartTransfer") {
      SECTION("Should retry the same segment (seq_index stays at 0)") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kCrcError)});

        task.Run();

        REQUIRE(sender.segment_call_count() == 2);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[1].seq_index == 0);
      }

      SECTION("Should stop the timer before retrying") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kCrcError)});

        task.Run();

        REQUIRE(timer.stop_count() == 1);
      }
    }

    SECTION("When kMaxRetries consecutive AckReceived(kCrcError) events are received") {
      SECTION("Should abandon after kMaxRetries retries and not send any more segments") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        for (std::uint8_t i = 0; i <= CalibrationTask::kMaxRetries; ++i) {
          queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kCrcError)});
        }

        task.Run();

        // Initial send + kMaxRetries retries; the last CrcError causes abandon (no send)
        REQUIRE(sender.segment_call_count() == CalibrationTask::kMaxRetries);
      }
    }

    SECTION("When a TimeoutTick event is received") {
      SECTION("Should retry the same segment") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        queue.Push(TimeoutTick{});

        task.Run();

        REQUIRE(sender.segment_call_count() == 2);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[1].seq_index == 0);
      }
    }

    SECTION("When kMaxRetries consecutive TimeoutTick events are received") {
      SECTION("Should abandon after kMaxRetries retries and not send any more segments") {
        queue.Push(StartTransfer{MakeCalibrationArray()});
        for (std::uint8_t i = 0; i <= CalibrationTask::kMaxRetries; ++i) {
          queue.Push(TimeoutTick{});
        }

        task.Run();

        // Initial send + kMaxRetries retries; the last timeout causes abandon
        REQUIRE(sender.segment_call_count() == CalibrationTask::kMaxRetries);
      }
    }

    SECTION("When packing the last segment (index kTotalSegments-1)") {
      SECTION("Trailing bytes beyond the last sensor should be zero") {
        // kSensorCount=22, kSensorsPerSegment=3 → segment 7 contains sensor[21] only.
        // Bytes [kSensorCalibrationSizeBytes..kPayloadSizeBytes-1] must be zero.
        CalibrationArray data{};
        data[21].rest_current_ma = 1.5f;
        data[21].strike_current_ma = 2.5f;
        data[21].rest_distance_mm = 3.5f;
        data[21].strike_distance_mm = 4.5f;

        queue.Push(StartTransfer{data});
        for (std::size_t i = 0; i < CalibrationTask::kTotalSegments - 1; ++i) {
          queue.Push(AckReceived{MakeAck(DataSegmentAckStatus::kOk)});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationTask::kTotalSegments);
        const auto& last_payload =
            sender.segment_calls()[CalibrationTask::kTotalSegments - 1].payload;
        constexpr std::size_t kSensorBytes =
            midismith::protocol::CalibrationDataSegment::kSensorCalibrationSizeBytes;
        for (std::size_t byte = kSensorBytes; byte < last_payload.size(); ++byte) {
          REQUIRE(last_payload[byte] == 0);
        }
      }
    }
  }
}

#endif
