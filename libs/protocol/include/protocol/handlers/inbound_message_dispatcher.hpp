#pragma once

#include <tuple>
#include <type_traits>
#include <variant>

#include "protocol/message_parser.hpp"
#include "protocol/messages.hpp"

namespace midismith::protocol::handlers {

template <typename T>
concept SensorEventHandler =
    requires(T& handler, const protocol::SensorEvent& event, std::uint8_t source_node_id) {
      { handler.OnSensorEvent(event, source_node_id) } noexcept;
    };

template <typename T>
concept HeartbeatHandler =
    requires(T& handler, const protocol::Heartbeat& heartbeat, std::uint8_t source_node_id) {
      { handler.OnHeartbeat(heartbeat, source_node_id) } noexcept;
    };

template <typename T>
concept AdcStartHandler = requires(T& handler, const protocol::AdcStart& command) {
  { handler.OnAdcStart(command) } noexcept;
};

template <typename T>
concept AdcStopHandler = requires(T& handler, const protocol::AdcStop& command) {
  { handler.OnAdcStop(command) } noexcept;
};

template <typename T>
concept CalibStartHandler = requires(T& handler, const protocol::CalibStart& command) {
  { handler.OnCalibStart(command) } noexcept;
};

template <typename T>
concept DumpRequestHandler =
    requires(T& handler, const protocol::DumpRequest& command, std::uint8_t source_node_id) {
      { handler.OnDumpRequest(command, source_node_id) } noexcept;
    };

template <typename T>
concept CalibrationDataSegmentHandler = requires(
    T& handler, const protocol::CalibrationDataSegment& segment, std::uint8_t source_node_id) {
  { handler.OnCalibrationDataSegment(segment, source_node_id) } noexcept;
};

template <typename T>
concept DataSegmentAckHandler =
    requires(T& handler, const protocol::DataSegmentAck& ack, std::uint8_t source_node_id) {
      { handler.OnDataSegmentAck(ack, source_node_id) } noexcept;
    };

template <typename... THandlers>
class InboundMessageDispatcher {
 public:
  explicit InboundMessageDispatcher(THandlers&... handlers) noexcept : handlers_(handlers...) {}

  [[nodiscard]] bool Dispatch(const protocol::IncomingMessage& message) noexcept {
    const std::uint8_t sender = std::visit(
        [](const auto& header) noexcept { return header.source_node_id; }, message.routing);
    return std::visit(
        [this, sender](const auto& content) noexcept {
          return DispatchTypedMessage(sender, content);
        },
        message.content);
  }

 private:
  std::tuple<THandlers&...> handlers_;

  [[nodiscard]] bool DispatchTypedMessage(std::uint8_t sender,
                                          const protocol::SensorEvent& event) noexcept {
    return DispatchToAllHandlers(sender, event);
  }

  [[nodiscard]] bool DispatchTypedMessage(std::uint8_t sender,
                                          const protocol::Heartbeat& heartbeat) noexcept {
    return DispatchToAllHandlers(sender, heartbeat);
  }

  [[nodiscard]] bool DispatchTypedMessage(
      std::uint8_t sender, const protocol::CalibrationDataSegment& segment) noexcept {
    return DispatchToAllHandlers(sender, segment);
  }

  [[nodiscard]] bool DispatchTypedMessage(std::uint8_t sender,
                                          const protocol::DataSegmentAck& ack) noexcept {
    return DispatchToAllHandlers(sender, ack);
  }

  [[nodiscard]] bool DispatchTypedMessage(std::uint8_t sender,
                                          const protocol::Command& command) noexcept {
    return std::visit(
        [this, sender](const auto& typed_command) noexcept {
          return DispatchTypedCommand(sender, typed_command);
        },
        command);
  }

  [[nodiscard]] bool DispatchTypedCommand(std::uint8_t /*sender*/,
                                          const protocol::AdcStart& command) noexcept {
    return DispatchToAllHandlers(command);
  }

  [[nodiscard]] bool DispatchTypedCommand(std::uint8_t /*sender*/,
                                          const protocol::AdcStop& command) noexcept {
    return DispatchToAllHandlers(command);
  }

  [[nodiscard]] bool DispatchTypedCommand(std::uint8_t /*sender*/,
                                          const protocol::CalibStart& command) noexcept {
    return DispatchToAllHandlers(command);
  }

  [[nodiscard]] bool DispatchTypedCommand(std::uint8_t sender,
                                          const protocol::DumpRequest& command) noexcept {
    return DispatchToAllHandlers(sender, command);
  }

  template <typename TMessage>
  [[nodiscard]] bool DispatchToAllHandlers(std::uint8_t sender, const TMessage& message) noexcept {
    bool did_dispatch = false;
    std::apply(
        [&message, sender, &did_dispatch](auto&... handlers) noexcept {
          ((did_dispatch = TryDispatch(handlers, message, sender) || did_dispatch), ...);
        },
        handlers_);
    return did_dispatch;
  }

  template <typename TMessage>
  [[nodiscard]] bool DispatchToAllHandlers(const TMessage& message) noexcept {
    bool did_dispatch = false;
    std::apply(
        [&message, &did_dispatch](auto&... handlers) noexcept {
          ((did_dispatch = TryDispatch(handlers, message) || did_dispatch), ...);
        },
        handlers_);
    return did_dispatch;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler, const protocol::SensorEvent& event,
                                        std::uint8_t source_node_id) noexcept {
    if constexpr (SensorEventHandler<THandler>) {
      handler.OnSensorEvent(event, source_node_id);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler, const protocol::Heartbeat& heartbeat,
                                        std::uint8_t source_node_id) noexcept {
    if constexpr (HeartbeatHandler<THandler>) {
      handler.OnHeartbeat(heartbeat, source_node_id);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::AdcStart& command) noexcept {
    if constexpr (AdcStartHandler<THandler>) {
      handler.OnAdcStart(command);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::AdcStop& command) noexcept {
    if constexpr (AdcStopHandler<THandler>) {
      handler.OnAdcStop(command);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::CalibStart& command) noexcept {
    if constexpr (CalibStartHandler<THandler>) {
      handler.OnCalibStart(command);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler, const protocol::DumpRequest& command,
                                        std::uint8_t source_node_id) noexcept {
    if constexpr (DumpRequestHandler<THandler>) {
      handler.OnDumpRequest(command, source_node_id);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::CalibrationDataSegment& segment,
                                        std::uint8_t source_node_id) noexcept {
    if constexpr (CalibrationDataSegmentHandler<THandler>) {
      handler.OnCalibrationDataSegment(segment, source_node_id);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler, const protocol::DataSegmentAck& ack,
                                        std::uint8_t source_node_id) noexcept {
    if constexpr (DataSegmentAckHandler<THandler>) {
      handler.OnDataSegmentAck(ack, source_node_id);
      return true;
    }
    return false;
  }
};

}  // namespace midismith::protocol::handlers
