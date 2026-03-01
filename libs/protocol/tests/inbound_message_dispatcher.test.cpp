#if defined(UNIT_TESTS)

#include "protocol/handlers/inbound_message_dispatcher.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

class StartStopCommandHandler final {
 public:
  void OnAdcStart(const midismith::protocol::AdcStart&) noexcept { ++start_calls; }

  void OnAdcStop(const midismith::protocol::AdcStop&) noexcept { ++stop_calls; }

  int start_calls = 0;
  int stop_calls = 0;
};

class StartOnlyCommandHandler final {
 public:
  void OnAdcStart(const midismith::protocol::AdcStart&) noexcept { ++start_calls; }

  int start_calls = 0;
};

}  // namespace

TEST_CASE("The InboundMessageDispatcher class", "[protocol][dispatcher]") {
  StartStopCommandHandler handler;
  midismith::protocol::handlers::InboundMessageDispatcher dispatcher(handler);

  SECTION("When dispatching an AdcStart command") {
    const midismith::protocol::IncomingMessage message =
        midismith::protocol::Command(midismith::protocol::AdcStart{});

    const bool did_dispatch = dispatcher.Dispatch(message);

    REQUIRE(did_dispatch);
    REQUIRE(handler.start_calls == 1);
    REQUIRE(handler.stop_calls == 0);
  }

  SECTION("When dispatching an AdcStop command") {
    const midismith::protocol::IncomingMessage message =
        midismith::protocol::Command(midismith::protocol::AdcStop{});

    const bool did_dispatch = dispatcher.Dispatch(message);

    REQUIRE(did_dispatch);
    REQUIRE(handler.start_calls == 0);
    REQUIRE(handler.stop_calls == 1);
  }

  SECTION("When dispatching an unhandled command type") {
    const midismith::protocol::IncomingMessage message =
        midismith::protocol::Command(
            midismith::protocol::CalibStart{.mode = midismith::protocol::CalibMode::kAuto});

    const bool did_dispatch = dispatcher.Dispatch(message);

    REQUIRE_FALSE(did_dispatch);
    REQUIRE(handler.start_calls == 0);
    REQUIRE(handler.stop_calls == 0);
  }

  SECTION("When dispatching a std::monostate message") {
    const midismith::protocol::IncomingMessage message = std::monostate{};

    const bool did_dispatch = dispatcher.Dispatch(message);

    REQUIRE_FALSE(did_dispatch);
    REQUIRE(handler.start_calls == 0);
    REQUIRE(handler.stop_calls == 0);
  }

  SECTION("When multiple handlers support the same message type") {
    StartOnlyCommandHandler first_handler;
    StartOnlyCommandHandler second_handler;
    midismith::protocol::handlers::InboundMessageDispatcher multi_dispatcher(first_handler,
                                                                             second_handler);
    const midismith::protocol::IncomingMessage message =
        midismith::protocol::Command(midismith::protocol::AdcStart{});

    const bool did_dispatch = multi_dispatcher.Dispatch(message);

    REQUIRE(did_dispatch);
    REQUIRE(first_handler.start_calls == 1);
    REQUIRE(second_handler.start_calls == 1);
  }
}

#endif
