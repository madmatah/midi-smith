#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "can-broker/can_frame_handler_requirements.hpp"
#include "protocol-can/can_mapper.hpp"
#include "protocol/message_parser.hpp"

namespace midismith::protocol_can {

struct CanInboundDecodeStats {
  std::uint32_t unknown_identifier_count = 0;
  std::uint32_t invalid_payload_count = 0;
  std::uint32_t dropped_message_count = 0;
};

template <typename TDispatcher>
class CanToProtocolAdapter final : public midismith::can_broker::CanFrameHandlerRequirements {
 public:
  explicit CanToProtocolAdapter(TDispatcher& dispatcher) noexcept : dispatcher_(dispatcher) {}

  void Handle(const midismith::bsp::can::FdcanFrame& frame) noexcept override {
    const auto decoded_header =
        CanIdentifierMapper::DecodeId(static_cast<std::uint16_t>(frame.identifier));
    if (!decoded_header.has_value()) {
      ++decode_stats_.unknown_identifier_count;
      return;
    }

    std::size_t payload_size_bytes = frame.data_length_bytes;
    if (payload_size_bytes > frame.data.size()) {
      payload_size_bytes = frame.data.size();
    }

    const auto decoded_message = midismith::protocol::MessageParser::Decode(
        *decoded_header, std::span<const std::uint8_t>(frame.data.data(), payload_size_bytes));
    if (!decoded_message.has_value()) {
      ++decode_stats_.invalid_payload_count;
      return;
    }

    if (!dispatcher_.Dispatch(*decoded_message)) {
      ++decode_stats_.dropped_message_count;
    }
  }

  [[nodiscard]] CanInboundDecodeStats CaptureDecodeStats() const noexcept {
    return decode_stats_;
  }

 private:
  TDispatcher& dispatcher_;
  CanInboundDecodeStats decode_stats_{};
};

}  // namespace midismith::protocol_can
