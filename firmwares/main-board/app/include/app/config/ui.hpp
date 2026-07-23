#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::main_board::app::config {

constexpr std::uint32_t kUiTaskStackBytes = 2048;
constexpr std::uint32_t kUiTaskPriority = 1;
constexpr std::uint32_t kUiTickPeriodMs = 20;
constexpr std::uint8_t kUiButtonDebounceReads = 3;
constexpr std::uint16_t kUiButtonLongPressReads = 50;
constexpr std::size_t kMenuStackMaxDepth = 6;
constexpr std::size_t kLineBufferMaxLines = 128;
constexpr std::size_t kLineBufferLineCapacity = 64;
constexpr std::uint8_t kTftTextColumns = 20;
constexpr std::uint8_t kTftTextRows = 8;
constexpr std::uint8_t kTftFontWidth = 8;
constexpr std::uint8_t kTftFontHeight = 16;

}  // namespace midismith::main_board::app::config
