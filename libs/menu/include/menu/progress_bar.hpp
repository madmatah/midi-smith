#pragma once

#include <cstdint>

#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

void RenderProgressBar(midismith::text_display::TextDisplayRequirements& display, std::uint8_t row,
                       std::uint8_t column, std::uint8_t width_cells, std::uint32_t value,
                       std::uint32_t maximum) noexcept;

}  // namespace midismith::menu
