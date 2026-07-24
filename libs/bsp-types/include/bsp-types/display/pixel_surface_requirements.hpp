#pragma once

#include <cstdint>

namespace midismith::bsp::display {

class PixelSurfaceRequirements {
 public:
  virtual ~PixelSurfaceRequirements() = default;

  virtual void BlitRows(std::uint16_t y, std::uint16_t row_count,
                        const std::uint8_t* pixel_bytes) noexcept = 0;
  virtual std::uint16_t width() const noexcept = 0;
  virtual std::uint16_t height() const noexcept = 0;
};

}  // namespace midismith::bsp::display
