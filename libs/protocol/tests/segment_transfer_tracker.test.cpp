#if defined(UNIT_TESTS)

#include "protocol/transfer/segment_transfer_tracker.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::protocol::transfer::SegmentTransferTracker;

}

TEST_CASE("The SegmentTransferTracker class", "[protocol][transfer]") {
  SECTION("The OnSegmentReceived() method") {
    SECTION("When segments are received in order") {
      SECTION("Should report completion after all expected segments arrive") {
        SegmentTransferTracker tracker;

        tracker.OnSegmentReceived(0u, 3u);
        REQUIRE_FALSE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 1u);

        tracker.OnSegmentReceived(1u, 3u);
        REQUIRE_FALSE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 2u);

        tracker.OnSegmentReceived(2u, 3u);
        REQUIRE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 3u);
      }
    }

    SECTION("When segments are received out of order") {
      SECTION("Should report completion once all unique indexes are present") {
        SegmentTransferTracker tracker;

        tracker.OnSegmentReceived(2u, 3u);
        REQUIRE_FALSE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 1u);

        tracker.OnSegmentReceived(0u, 3u);
        REQUIRE_FALSE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 2u);

        tracker.OnSegmentReceived(1u, 3u);
        REQUIRE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 3u);
      }
    }

    SECTION("When the same segment is received multiple times") {
      SECTION("Should count it only once") {
        SegmentTransferTracker tracker;

        tracker.OnSegmentReceived(0u, 2u);
        tracker.OnSegmentReceived(0u, 2u);

        REQUIRE(tracker.received_count() == 1u);
        REQUIRE_FALSE(tracker.IsComplete());
      }
    }
  }

  SECTION("The Reset() method") {
    SECTION("When tracker has pending segments") {
      SECTION("Should clear all state") {
        SegmentTransferTracker tracker;
        tracker.OnSegmentReceived(0u, 2u);
        tracker.Reset();

        REQUIRE_FALSE(tracker.IsComplete());
        REQUIRE(tracker.received_count() == 0u);
      }
    }
  }
}

#endif
