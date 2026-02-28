#if defined(UNIT_TESTS)

#include "can-broker/can_task.hpp"

#include <catch2/catch_test_macros.hpp>
#include <vector>

namespace {

struct RecordingCanFrameHandler final : public midismith::can_broker::CanFrameHandlerRequirements {
  std::vector<midismith::bsp::can::FdcanFrame> received_frames;

  void Handle(const midismith::bsp::can::FdcanFrame& frame) noexcept override {
    received_frames.push_back(frame);
  }
};

struct FixedQueueStub final
    : public midismith::os::QueueRequirements<midismith::bsp::can::FdcanFrame> {
  explicit FixedQueueStub(std::vector<midismith::bsp::can::FdcanFrame> frames_to_deliver)
      : frames_(std::move(frames_to_deliver)) {}

  bool Send(const midismith::bsp::can::FdcanFrame&, std::uint32_t) noexcept override {
    return false;
  }

  bool SendFromIsr(const midismith::bsp::can::FdcanFrame&) noexcept override { return false; }

  bool Receive(midismith::bsp::can::FdcanFrame& item, std::uint32_t) noexcept override {
    if (next_index_ >= frames_.size()) {
      return false;
    }
    item = frames_[next_index_++];
    return true;
  }

 private:
  std::vector<midismith::bsp::can::FdcanFrame> frames_;
  std::size_t next_index_ = 0;
};

}  // namespace

TEST_CASE("CanTask") {
  SECTION("Run()") {
    SECTION("When the queue delivers frames") {
      SECTION("Should forward each frame to the handler in order") {
        midismith::bsp::can::FdcanFrame frame_a{};
        frame_a.identifier = 0x10;
        frame_a.data_length_bytes = 1;
        frame_a.data[0] = 0xAA;

        midismith::bsp::can::FdcanFrame frame_b{};
        frame_b.identifier = 0x20;
        frame_b.data_length_bytes = 2;
        frame_b.data[0] = 0xBB;
        frame_b.data[1] = 0xCC;

        FixedQueueStub queue({frame_a, frame_b});
        RecordingCanFrameHandler handler;
        midismith::can_broker::CanTask task(queue, handler);

        task.Run();

        REQUIRE(handler.received_frames.size() == 2);
        REQUIRE(handler.received_frames[0].identifier == 0x10);
        REQUIRE(handler.received_frames[0].data[0] == 0xAA);
        REQUIRE(handler.received_frames[1].identifier == 0x20);
        REQUIRE(handler.received_frames[1].data[0] == 0xBB);
        REQUIRE(handler.received_frames[1].data[1] == 0xCC);
      }
    }

    SECTION("When the queue is immediately empty") {
      SECTION("Should not call the handler") {
        FixedQueueStub queue({});
        RecordingCanFrameHandler handler;
        midismith::can_broker::CanTask task(queue, handler);

        task.Run();

        REQUIRE(handler.received_frames.empty());
      }
    }
  }
}

#endif
