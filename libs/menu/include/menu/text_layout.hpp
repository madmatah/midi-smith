#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::menu {

constexpr std::uint8_t CenteredColumn(std::uint8_t columns, std::size_t text_length) noexcept {
  return text_length >= columns ? 0 : static_cast<std::uint8_t>((columns - text_length) / 2);
}

}  // namespace midismith::menu
