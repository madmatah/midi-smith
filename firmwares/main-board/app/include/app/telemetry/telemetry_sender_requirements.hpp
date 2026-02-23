#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::main_board::app::telemetry {

class TelemetrySenderRequirements {
 public:
  virtual ~TelemetrySenderRequirements() = default;

  virtual std::size_t Send(std::span<const std::uint8_t> data) noexcept = 0;

  template <typename T>
  std::size_t Send(const T& value) noexcept {
    return Send(
        std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(&value), sizeof(T)));
  }
};

}  // namespace midismith::main_board::app::telemetry
