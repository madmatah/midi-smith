#if defined(UNIT_TESTS)

#include "app/calibration/calibration_data_server_task.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "app/calibration/calibration_save_completion_requirements.hpp"
#include "app/storage/calibration_persistent_store.hpp"
#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "domain/calibration/calibration_data.hpp"
#include "domain/config/main_board_config.hpp"
#include "os-types/queue_requirements.hpp"
#include "os-types/timer_requirements.hpp"
#include "protocol-can/can_calibration_segment_packer.hpp"
#include "protocol/messages.hpp"

namespace {

using CalibrationDataServerTask =
    midismith::main_board::app::calibration::CalibrationDataServerTask;
using Event = CalibrationDataServerTask::Event;
using LoadRequest = CalibrationDataServerTask::LoadRequest;
using AckReceived = CalibrationDataServerTask::AckReceived;
using AckTimeout = CalibrationDataServerTask::AckTimeout;
using CalibrationPersistentStore = midismith::main_board::app::storage::CalibrationPersistentStore;
using CalibrationData = midismith::main_board::domain::calibration::CalibrationData;
using SegmentPayload = CalibrationDataServerTask::SegmentPayload;
using DataSegmentAck = midismith::protocol::DataSegmentAck;
using DataSegmentAckStatus = midismith::protocol::DataSegmentAckStatus;
using StorageOperationResult = midismith::bsp::storage::StorageOperationResult;

inline constexpr std::size_t kSensorsPerBoard =
    midismith::main_board::domain::config::kSensorsPerBoard;

class RecordingMessageSender final
    : public midismith::main_board::app::messaging::MainBoardMessageSenderRequirements {
 public:
  struct SegmentCall {
    std::uint8_t target_node_id;
    std::uint8_t seq_index;
    std::uint8_t total_packets;
    SegmentPayload payload;
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

  bool SendCalibrationAck(std::uint8_t, std::uint8_t, DataSegmentAckStatus) noexcept override {
    return true;
  }

  bool SendCalibrationDataSegment(
      std::uint8_t target_node_id, std::uint8_t seq_index, std::uint8_t total_packets,
      const std::array<std::uint8_t,
                       midismith::protocol::CalibrationDataSegment::kPayloadSizeBytes>&
          payload) noexcept override {
    segment_calls_.push_back({target_node_id, seq_index, total_packets, payload});
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

  [[nodiscard]] const std::optional<std::uint32_t>& last_period_ms() const noexcept {
    return last_period_ms_;
  }

 private:
  std::uint32_t start_count_ = 0;
  std::uint32_t stop_count_ = 0;
  std::optional<std::uint32_t> last_period_ms_;
};

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  StorageOperationResult Read(std::size_t offset_bytes, std::uint8_t* buffer,
                              std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return StorageOperationResult::kSuccess;
  }

  StorageOperationResult EraseSector() noexcept override {
    std::memset(storage_, 0xFF, sizeof(storage_));
    return StorageOperationResult::kSuccess;
  }

  StorageOperationResult Write(std::size_t offset_bytes, const std::uint8_t* data,
                               std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    return StorageOperationResult::kSuccess;
  }

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

CalibrationData MakeCalibrationDataForBoard(std::uint8_t board_id) noexcept {
  CalibrationData calibration_data{};
  const std::size_t board_index = static_cast<std::size_t>(board_id - 1u);
  calibration_data.board_data_valid[board_index] = true;

  for (std::size_t sensor_index = 0; sensor_index < kSensorsPerBoard; ++sensor_index) {
    auto& calibration = calibration_data.sensor_calibrations[board_index][sensor_index];
    calibration.rest_current_ma =
        static_cast<float>(board_id) * 100.0f + static_cast<float>(sensor_index);
    calibration.strike_current_ma =
        static_cast<float>(board_id) * 10.0f + static_cast<float>(sensor_index);
    calibration.rest_distance_mm = 7.0f + static_cast<float>(sensor_index);
    calibration.strike_distance_mm = 2.0f + static_cast<float>(sensor_index);
  }

  return calibration_data;
}

SegmentPayload MakeExpectedPayload(const CalibrationData& calibration_data, std::uint8_t board_id,
                                   std::size_t segment_index) noexcept {
  SegmentPayload payload{};
  const auto& board_calibration = calibration_data.sensor_calibrations[board_id - 1u];
  midismith::protocol_can::CanCalibrationSegmentPacker::PackSegment(
      board_calibration.data(), board_calibration.size(), segment_index, payload.data());
  return payload;
}

DataSegmentAck MakeAck(std::uint8_t ack_index, DataSegmentAckStatus status) noexcept {
  DataSegmentAck ack{};
  ack.ack_index = ack_index;
  ack.status = status;
  return ack;
}

class FakeSaveCompletionCallback final
    : public midismith::main_board::app::calibration::CalibrationSaveCompletionRequirements {
 public:
  void OnSaveComplete() noexcept override {
    ++completion_count;
  }

  int completion_count = 0;
};

}  // namespace

TEST_CASE("The CalibrationDataServerTask class") {
  RecordingMessageSender sender;
  StubEventQueue queue;
  RecordingTimer timer;
  FlashStorageStub flash_storage;
  CalibrationPersistentStore store(flash_storage);
  CalibrationDataServerTask task(sender, store, queue, timer);

  SECTION("The OnAckTimeout() static method") {
    SECTION("When called with a valid queue pointer") {
      SECTION("Should post an AckTimeout event to the queue") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});

        CalibrationDataServerTask::OnAckTimeout(&queue);

        task.Run();

        REQUIRE(sender.segment_call_count() >= 2);
      }
    }

    SECTION("When called with a null pointer") {
      SECTION("Should not send any segment") {
        CalibrationDataServerTask::OnAckTimeout(nullptr);

        task.Run();

        REQUIRE(sender.segment_call_count() == 0);
      }
    }
  }

  SECTION("The Run() method") {
    SECTION("When a valid LoadRequest is received") {
      SECTION("Should send segment zero for the requested board") {
        const auto calibration_data = MakeCalibrationDataForBoard(2);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{2});

        task.Run();

        REQUIRE(sender.segment_call_count() == 1);
        REQUIRE(sender.segment_calls()[0].target_node_id == 2);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[0].total_packets ==
                CalibrationDataServerTask::kTotalSegments);
        REQUIRE(sender.segment_calls()[0].payload == MakeExpectedPayload(calibration_data, 2, 0));
      }
    }

    SECTION("When a valid LoadRequest starts a transfer") {
      SECTION("Should start the ack timer") {
        const auto calibration_data = MakeCalibrationDataForBoard(3);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{3});

        task.Run();

        REQUIRE(timer.start_count() == 1);
      }
    }

    SECTION("When a LoadRequest targets a board without stored data") {
      SECTION("Should send a single no-data segment and not start the timer") {
        queue.Push(LoadRequest{4});

        task.Run();

        REQUIRE(sender.segment_call_count() == 1);
        REQUIRE(sender.segment_calls()[0].target_node_id == 4);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[0].total_packets == 0);
        REQUIRE(timer.start_count() == 0);
      }
    }

    SECTION("When all expected kOk acknowledgements are received") {
      SECTION("Should send all segments in sequence and complete the transfer") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        for (std::size_t segment_index = 0;
             segment_index < CalibrationDataServerTask::kTotalSegments; ++segment_index) {
          queue.Push(AckReceived{
              .source_node_id = 1,
              .ack = MakeAck(static_cast<std::uint8_t>(segment_index), DataSegmentAckStatus::kOk)});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationDataServerTask::kTotalSegments);
        for (std::size_t segment_index = 0;
             segment_index < CalibrationDataServerTask::kTotalSegments; ++segment_index) {
          REQUIRE(sender.segment_calls()[segment_index].target_node_id == 1);
          REQUIRE(sender.segment_calls()[segment_index].seq_index ==
                  static_cast<std::uint8_t>(segment_index));
        }
        REQUIRE(timer.stop_count() == CalibrationDataServerTask::kTotalSegments);
      }
    }

    SECTION("When an error acknowledgement is received for the current segment") {
      SECTION("Should retry the same segment") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        queue.Push(
            AckReceived{.source_node_id = 1, .ack = MakeAck(0, DataSegmentAckStatus::kCrcError)});

        task.Run();

        REQUIRE(sender.segment_call_count() == 2);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[1].seq_index == 0);
        REQUIRE(timer.stop_count() == 1);
      }
    }

    SECTION("When kMaxRetries error acknowledgements are received") {
      SECTION("Should abandon the transfer after the retry budget is exhausted") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        for (std::uint8_t retry_index = 0; retry_index <= CalibrationDataServerTask::kMaxRetries;
             ++retry_index) {
          queue.Push(
              AckReceived{.source_node_id = 1, .ack = MakeAck(0, DataSegmentAckStatus::kCrcError)});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationDataServerTask::kMaxRetries);
      }
    }

    SECTION("When an AckTimeout event is received during an active transfer") {
      SECTION("Should retry the current segment") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        queue.Push(AckTimeout{});

        task.Run();

        REQUIRE(sender.segment_call_count() == 2);
        REQUIRE(sender.segment_calls()[0].seq_index == 0);
        REQUIRE(sender.segment_calls()[1].seq_index == 0);
      }
    }

    SECTION("When kMaxRetries timeout events are received") {
      SECTION("Should abandon the transfer after the retry budget is exhausted") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        for (std::uint8_t retry_index = 0; retry_index <= CalibrationDataServerTask::kMaxRetries;
             ++retry_index) {
          queue.Push(AckTimeout{});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationDataServerTask::kMaxRetries);
      }
    }

    SECTION("When an acknowledgement comes from another board") {
      SECTION("Should ignore it and keep waiting for the active board") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        queue.Push(AckReceived{.source_node_id = 2, .ack = MakeAck(0, DataSegmentAckStatus::kOk)});

        task.Run();

        REQUIRE(sender.segment_call_count() == 1);
        REQUIRE(timer.stop_count() == 0);
      }
    }

    SECTION("When an acknowledgement index does not match the current segment") {
      SECTION("Should ignore it and keep waiting for the expected acknowledgement") {
        const auto calibration_data = MakeCalibrationDataForBoard(1);
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        queue.Push(AckReceived{.source_node_id = 1, .ack = MakeAck(1, DataSegmentAckStatus::kOk)});

        task.Run();

        REQUIRE(sender.segment_call_count() == 1);
        REQUIRE(timer.stop_count() == 0);
      }
    }

    SECTION("When a SaveRequest is received") {
      SECTION("Should call store.Save() then invoke the completion callback") {
        CalibrationData data_to_save = MakeCalibrationDataForBoard(1);
        FakeSaveCompletionCallback completion;
        task.RequestSave(data_to_save, completion);

        task.Run();

        REQUIRE(completion.completion_count == 1);
        const auto* cached = store.cached_calibration();
        REQUIRE(cached != nullptr);
        REQUIRE(cached->board_data_valid[0] == true);
      }
    }

    SECTION("When multiple LoadRequest events arrive close together") {
      SECTION("Should serve the boards sequentially") {
        auto calibration_data = MakeCalibrationDataForBoard(1);
        const auto calibration_data_for_second_board = MakeCalibrationDataForBoard(2);
        calibration_data.sensor_calibrations[1] =
            calibration_data_for_second_board.sensor_calibrations[1];
        calibration_data.board_data_valid[1] = true;
        REQUIRE(store.Save(calibration_data) == StorageOperationResult::kSuccess);
        queue.Push(LoadRequest{1});
        queue.Push(LoadRequest{2});
        for (std::size_t segment_index = 0;
             segment_index < CalibrationDataServerTask::kTotalSegments; ++segment_index) {
          queue.Push(AckReceived{
              .source_node_id = 1,
              .ack = MakeAck(static_cast<std::uint8_t>(segment_index), DataSegmentAckStatus::kOk)});
        }

        task.Run();

        REQUIRE(sender.segment_call_count() == CalibrationDataServerTask::kTotalSegments + 1);
        REQUIRE(sender.segment_calls().back().target_node_id == 2);
        REQUIRE(sender.segment_calls().back().seq_index == 0);
      }
    }
  }
}

#endif
