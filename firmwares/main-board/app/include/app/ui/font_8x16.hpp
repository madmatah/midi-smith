#pragma once

#include <cstdint>
#include <span>

namespace midismith::main_board::app::ui {

std::span<const std::uint8_t, 16> Font8x16Glyph(char character) noexcept;

}
