#if defined(UNIT_TESTS)

#include "protocol/handlers/inbound_message_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::protocol::UnicastTransportHeader;
using midismith::protocol::BroadcastTransportHeader;
using midismith::protocol::MessageCategory;
using midismith::protocol::MessageType;
using midismith::protocol::IncomingMessage;

IncomingMessage MakeUnicastMessage(std::variant<midismith::protocol::SensorEvent,
                                               midismith::protocol::Command,
                                               midismith::protocol::Heartbeat> content,
                                   std::uint8_t source_node_id = 1) {
  return {.routing = UnicastTransportHeader::Make(MessageCategory::kRealTime,
                                                  MessageType::kSensorEvent, source_node_id, 0),
          .content = std::move(content)};
}

IncomingMessage MakeBroadcastMessage(std::variant<midismith::protocol::SensorEvent,
                                                  midismith::protocol::Command,
                                                  midismith::protocol::Heartbeat> content) {
  return {.routing = BroadcastTransportHeader::Make(MessageCategory::kSystem,
                                                    MessageType::kHeartbeat, 0),
          .content = std::move(content)};
}

class RecordingSensorEventHandler final {
 public:
  void OnSensorEvent(const midismith::protocol::SensorEvent&,
                     std::uint8_t source_node_id) noexcept {
    ++calls;
    last_source_node_id = source_node_id;
  }
  int calls = 0;
  std::uint8_t last_source_node_id = 0xFF;
};

class RecordingHeartbeatHandler final {
 public:
  void OnHeartbeat(const midismith::protocol::Heartbeat&, std::uint8_t source_node_id) noexcept {
    ++calls;
    last_source_node_id = source_node_id;
  }
  int calls = 0;
  std::uint8_t last_source_node_id = 0xFF;
};

class RecordingAdcCommandHandler final {
 public:
  void OnAdcStart(const midismith::protocol::AdcStart&) noexcept { ++start_calls; }
  void OnAdcStop(const midismith::protocol::AdcStop&) noexcept { ++stop_calls; }
  int start_calls = 0;
  int stop_calls = 0;
};

class RecordingCalibCommandHandler final {
 public:
  void OnCalibStart(const midismith::protocol::CalibStart&) noexcept { ++calls; }
  int calls = 0;
};

class RecordingDumpRequestHandler final {
 public:
  void OnDumpRequest(const midismith::protocol::DumpRequest&, std::uint8_t source_node_id) noexcept {
    ++calls;
    last_source_node_id = source_node_id;
  }
  int calls = 0;
  std::uint8_t last_source_node_id = 0xFF;
};

}  // namespace

TEST_CASE("The InboundMessageDispatcher class", "[protocol][dispatcher]") {
  SECTION("The Dispatch() method") {
    SECTION("When dispatching a SensorEvent") {
      SECTION("Should notify the handler with the correct source_node_id") {
        RecordingSensorEventHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message = MakeUnicastMessage(midismith::protocol::SensorEvent{}, 3);

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
        REQUIRE(handler.last_source_node_id == 3);
      }
    }

    SECTION("When dispatching a Heartbeat") {
      SECTION("Should notify the handler with the correct source_node_id") {
        RecordingHeartbeatHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message = MakeUnicastMessage(midismith::protocol::Heartbeat{}, 5);

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
        REQUIRE(handler.last_source_node_id == 5);
      }

      SECTION("When dispatched from a broadcast routing") {
        SECTION("Should use source_node_id from the broadcast header") {
          RecordingHeartbeatHandler handler;
          midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
          const auto message = MakeBroadcastMessage(midismith::protocol::Heartbeat{});

          const bool result = dispatcher.Dispatch(message);

          REQUIRE(result);
          REQUIRE(handler.last_source_node_id == 0);
        }
      }
    }

    SECTION("When dispatching an AdcStart command") {
      SECTION("Should notify the command handler") {
        RecordingAdcCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message =
            MakeUnicastMessage(midismith::protocol::Command(midismith::protocol::AdcStart{}));

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.start_calls == 1);
      }
    }

    SECTION("When dispatching an AdcStop command") {
      SECTION("Should notify the command handler") {
        RecordingAdcCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message =
            MakeUnicastMessage(midismith::protocol::Command(midismith::protocol::AdcStop{}));

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.stop_calls == 1);
      }
    }

    SECTION("When dispatching a CalibStart command") {
      SECTION("Should notify the calibration handler") {
        RecordingCalibCommandHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message =
            MakeUnicastMessage(midismith::protocol::Command(midismith::protocol::CalibStart{}));

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
      }
    }

    SECTION("When dispatching a DumpRequest command") {
      SECTION("Should notify the handler with the correct source_node_id") {
        RecordingDumpRequestHandler handler;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);
        const auto message =
            MakeUnicastMessage(midismith::protocol::Command(midismith::protocol::DumpRequest{}), 2);

        const bool result = dispatcher.Dispatch(message);

        REQUIRE(result);
        REQUIRE(handler.calls == 1);
        REQUIRE(handler.last_source_node_id == 2);
      }
    }

    SECTION("When multiple handlers are registered") {
      SECTION("Should notify all handlers that support the message type") {
        RecordingAdcCommandHandler first;
        RecordingAdcCommandHandler second;
        midismith::protocol::handlers::InboundMessageDispatcher dispatcher(first, second);
        const auto message =
            MakeUnicastMessage(midismith::protocol::Command(midismith::protocol::AdcStart{}));

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
        const auto message = MakeUnicastMessage(midismith::protocol::Heartbeat{});

        const bool result = dispatcher.Dispatch(message);

        REQUIRE_FALSE(result);
        REQUIRE(handler.calls == 0);
      }
    }
  }
}

#endif
