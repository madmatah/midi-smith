#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::main_board::app::ui {

inline constexpr std::size_t kSlideAnimationSteps = 10;

enum class SlideDirection : std::uint8_t {
  kNone,
  kLeft,
  kRight,
};

// Quadratic ease-out in integer math: offset = width * t * (2 - t), t = (step + 1) / steps.
constexpr std::uint16_t SlideOffset(std::uint16_t width, std::size_t step) noexcept {
  const std::uint32_t step_index = static_cast<std::uint32_t>(step) + 1;
  return static_cast<std::uint16_t>(static_cast<std::uint32_t>(width) * step_index *
                                    (2 * kSlideAnimationSteps - step_index) /
                                    (kSlideAnimationSteps * kSlideAnimationSteps));
}

inline void ComposeSlideRow(std::uint16_t* destination, const std::uint16_t* previous_row,
                            const std::uint16_t* next_row, std::uint16_t width,
                            std::uint16_t offset, SlideDirection direction) noexcept {
  const std::uint16_t clamped_offset = offset > width ? width : offset;
  const std::uint16_t remaining_width = static_cast<std::uint16_t>(width - clamped_offset);
  if (direction == SlideDirection::kLeft) {
    for (std::uint16_t x = 0; x < remaining_width; x++) {
      destination[x] = previous_row[x + clamped_offset];
    }
    for (std::uint16_t x = 0; x < clamped_offset; x++) {
      destination[remaining_width + x] = next_row[x];
    }
    return;
  }
  for (std::uint16_t x = 0; x < clamped_offset; x++) {
    destination[x] = next_row[remaining_width + x];
  }
  for (std::uint16_t x = 0; x < remaining_width; x++) {
    destination[clamped_offset + x] = previous_row[x];
  }
}

}  // namespace midismith::main_board::app::ui
