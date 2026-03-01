#pragma once

#include <tuple>
#include <type_traits>
#include <variant>

#include "protocol/message_parser.hpp"
#include "protocol/messages.hpp"

namespace midismith::protocol::handlers {

template <typename T>
concept SensorEventHandler = requires(T& handler, const protocol::SensorEvent& event) {
  { handler.OnSensorEvent(event) } noexcept;
};

template <typename T>
concept HeartbeatHandler = requires(T& handler, const protocol::Heartbeat& heartbeat) {
  { handler.OnHeartbeat(heartbeat) } noexcept;
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
concept DumpRequestHandler = requires(T& handler, const protocol::DumpRequest& command) {
  { handler.OnDumpRequest(command) } noexcept;
};

template <typename... THandlers>
class InboundMessageDispatcher {
 public:
  explicit InboundMessageDispatcher(THandlers&... handlers) noexcept : handlers_(handlers...) {}

  [[nodiscard]] bool Dispatch(const protocol::IncomingMessage& message) noexcept {
    return std::visit(
        [this](const auto& typed_message) noexcept { return DispatchTypedMessage(typed_message); },
        message);
  }

 private:
  std::tuple<THandlers&...> handlers_;

  [[nodiscard]] bool DispatchTypedMessage(const std::monostate&) const noexcept {
    return false;
  }

  [[nodiscard]] bool DispatchTypedMessage(const protocol::SensorEvent& event) noexcept {
    return DispatchToAllHandlers(event);
  }

  [[nodiscard]] bool DispatchTypedMessage(const protocol::Heartbeat& heartbeat) noexcept {
    return DispatchToAllHandlers(heartbeat);
  }

  [[nodiscard]] bool DispatchTypedMessage(const protocol::Command& command) noexcept {
    return std::visit(
        [this](const auto& typed_command) noexcept { return DispatchToAllHandlers(typed_command); },
        command);
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
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::SensorEvent& event) noexcept {
    if constexpr (SensorEventHandler<THandler>) {
      handler.OnSensorEvent(event);
      return true;
    }
    return false;
  }

  template <typename THandler>
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::Heartbeat& heartbeat) noexcept {
    if constexpr (HeartbeatHandler<THandler>) {
      handler.OnHeartbeat(heartbeat);
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
  [[nodiscard]] static bool TryDispatch(THandler& handler,
                                        const protocol::DumpRequest& command) noexcept {
    if constexpr (DumpRequestHandler<THandler>) {
      handler.OnDumpRequest(command);
      return true;
    }
    return false;
  }
};

}  // namespace midismith::protocol::handlers
