#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::bsp {

class RttTelemetrySender final {
 public:
  RttTelemetrySender(unsigned channel, const char* name, void* buffer, unsigned size) noexcept;

  std::size_t Send(std::span<const std::uint8_t> data) noexcept;

 private:
  unsigned _channel;
};

}  // namespace midismith::bsp
