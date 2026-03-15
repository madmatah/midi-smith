#if defined(UNIT_TESTS)

#include "protocol-can/can_to_protocol_adapter.hpp"

#include <array>
#include <optional>
#include <span>

#include <catch2/catch_test_macros.hpp>

#include "protocol-can/can_mapper.hpp"
#include "protocol/builders.hpp"
#include "protocol/message_parser.hpp"
#include "protocol/messages.hpp"

namespace {

struct DispatchRecorder final {
  bool dispatch_result = true;
  int dispatch_call_count = 0;
  std::optional<midismith::protocol::IncomingMessage> last_message = std::nullopt;

  bool Dispatch(const midismith::protocol::IncomingMessage& message) noexcept {
    ++dispatch_call_count;
    last_message = message;
    return dispatch_result;
  }
};

midismith::bsp::can::FdcanFrame BuildStartAdcFrame(std::uint8_t target_node_id) {
  midismith::bsp::can::FdcanFrame frame{};

  const auto [header, command] =
      midismith::protocol::MainBoardMessageBuilder().BuildStartAdc(target_node_id);
  frame.identifier = midismith::protocol_can::CanIdentifierMapper::EncodeId(header);

  std::array<std::uint8_t, midismith::bsp::can::kCanFdMaxDataBytes> payload_buffer{};
  const auto payload_size_bytes =
      midismith::protocol::Serialize(command, std::span(payload_buffer));
  if (payload_size_bytes.has_value()) {
    frame.data = payload_buffer;
    frame.data_length_bytes = *payload_size_bytes;
  }

  return frame;
}

}  // namespace

TEST_CASE("The CanToProtocolAdapter class", "[protocol-can][adapter]") {
  DispatchRecorder dispatcher;
  midismith::protocol_can::CanToProtocolAdapter<DispatchRecorder> adapter(dispatcher);

  SECTION("When handling a frame with an unknown identifier") {
    midismith::bsp::can::FdcanFrame frame{};
    frame.identifier = 0x000;

    adapter.Handle(frame);
    const auto stats = adapter.CaptureDecodeStats();

    REQUIRE(dispatcher.dispatch_call_count == 0);
    REQUIRE(stats.dispatched_message_count == 0);
    REQUIRE(stats.unknown_identifier_count == 1);
    REQUIRE(stats.invalid_payload_count == 0);
    REQUIRE(stats.dropped_message_count == 0);
  }

  SECTION("When handling a frame with a valid identifier but an invalid payload") {
    auto frame = BuildStartAdcFrame(2);
    frame.data_length_bytes = 0;

    adapter.Handle(frame);
    const auto stats = adapter.CaptureDecodeStats();

    REQUIRE(dispatcher.dispatch_call_count == 0);
    REQUIRE(stats.dispatched_message_count == 0);
    REQUIRE(stats.unknown_identifier_count == 0);
    REQUIRE(stats.invalid_payload_count == 1);
    REQUIRE(stats.dropped_message_count == 0);
  }

  SECTION("When handling a valid frame and the dispatcher handles the message") {
    auto frame = BuildStartAdcFrame(2);
    dispatcher.dispatch_result = true;

    adapter.Handle(frame);
    const auto stats = adapter.CaptureDecodeStats();

    REQUIRE(dispatcher.dispatch_call_count == 1);
    REQUIRE(dispatcher.last_message.has_value());
    auto* command = std::get_if<midismith::protocol::Command>(&dispatcher.last_message->content);
    REQUIRE(command != nullptr);
    REQUIRE(std::get_if<midismith::protocol::AdcStart>(command) != nullptr);
    REQUIRE(stats.dispatched_message_count == 1);
    REQUIRE(stats.unknown_identifier_count == 0);
    REQUIRE(stats.invalid_payload_count == 0);
    REQUIRE(stats.dropped_message_count == 0);
  }

  SECTION("When handling a valid frame and the dispatcher does not handle the message") {
    auto frame = BuildStartAdcFrame(3);
    dispatcher.dispatch_result = false;

    adapter.Handle(frame);
    const auto stats = adapter.CaptureDecodeStats();

    REQUIRE(dispatcher.dispatch_call_count == 1);
    REQUIRE(stats.dispatched_message_count == 0);
    REQUIRE(stats.unknown_identifier_count == 0);
    REQUIRE(stats.invalid_payload_count == 0);
    REQUIRE(stats.dropped_message_count == 1);
  }
}

#endif
