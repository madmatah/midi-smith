#if defined(UNIT_TESTS)

#include "protocol/handlers/inbound_message_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

class RecordingSensorEventHandler final {
 public:
  void OnSensorEvent(const midismith::protocol::SensorEvent&) noexcept {
    ++calls;
  }
  int calls = 0;
};

class RecordingHeartbeatHandler final {
 public:
  void OnHeartbeat(const midismith::protocol::Heartbeat&) noexcept {
    ++calls;
  }
  int calls = 0;
};

class RecordingAdcCommandHandler final {
 public:
  void OnAdcStart(const midismith::protocol::AdcStart&) noexcept {
    ++start_calls;
  }
  void OnAdcStop(const midismith::protocol::AdcStop&) noexcept {
    ++stop_calls;
  }
  int start_calls = 0;
  int stop_calls = 0;
};

class RecordingCalibCommandHandler final {
 public:
  void OnCalibStart(const midismith::protocol::CalibStart&) noexcept {
    ++calls;
  }
  int calls = 0;
};

class RecordingDumpRequestHandler final {
 public:
  void OnDumpRequest(const midismith::protocol::DumpRequest&) noexcept {
    ++calls;
  }
  int calls = 0;
};

}  // namespace

TEST_CASE("The InboundMessageDispatcher class", "[protocol][dispatcher]") {
  SECTION("The Dispatch() method") {
    SECTION("When dispatching a SensorEvent") {
      SECTION("Should notify the sensor event handler") {
        RecordingSensorEventHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message = midismith::protocol::SensorEvent{};

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
      }
    }

    SECTION("When dispatching a Heartbeat") {
      SECTION("Should notify the heartbeat handler") {
        RecordingHeartbeatHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message = midismith::protocol::Heartbeat{};

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
      }
    }

    SECTION("When dispatching an AdcStart command") {
      SECTION("Should notify the command handler") {
        RecordingAdcCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message =
            midismith::protocol::Command(midismith::protocol::AdcStart{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.start_calls == 1);
      }
    }

    SECTION("When dispatching an AdcStop command") {
      SECTION("Should notify the command handler") {
        RecordingAdcCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message =
            midismith::protocol::Command(midismith::protocol::AdcStop{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.stop_calls == 1);
      }
    }

    SECTION("When dispatching a CalibStart command") {
      SECTION("Should notify the calibration handler") {
        RecordingCalibCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message =
            midismith::protocol::Command(midismith::protocol::CalibStart{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
      }
    }

    SECTION("When dispatching a DumpRequest command") {
      SECTION("Should notify the dump request handler") {
        RecordingDumpRequestHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message =
            midismith::protocol::Command(midismith::protocol::DumpRequest{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
      }
    }

    SECTION("When multiple handlers are registered") {
      SECTION("Should notify all handlers that support the message type") {
        RecordingAdcCommandHandler first;
        RecordingAdcCommandHandler second;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(first, second);
        const midismith::protocol::IncomingMessage message =
            midismith::protocol::Command(midismith::protocol::AdcStart{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(first.start_calls == 1);
        REQUIRE(second.start_calls == 1);
      }
    }

    SECTION("When no handler supports the message type") {
      SECTION("Should return false and not notify anyone") {
        RecordingSensorEventHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message = midismith::protocol::Heartbeat{};

        const bool result = dispatcher.Dispatch(message);

        REQUIRE_FALSE(result);
        REQUIRE(handler.calls == 0);
      }
    }

    SECTION("When dispatching a std::monostate") {
      SECTION("Should return false") {
        RecordingSensorEventHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const midismith::protocol::IncomingMessage message = std::monostate{};

        const bool result = dispatcher.Dispatch(message);

        REQUIRE_FALSE(result);
      }
    }
  }
}

#endif
