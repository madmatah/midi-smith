#pragma once

#include <cstddef>
#include <cstdint>

namespace midismith::main_board::app::config {

constexpr std::uint32_t kUiTaskStackBytes = 6144;
constexpr std::uint32_t kUiTaskPriority = 1;
constexpr std::uint32_t kUiTickPeriodMs = 20;
constexpr std::uint32_t kSplashFramePeriodMs = 33;
constexpr int kSplashBandRows = 8;
constexpr int kSplashSaturationPercent = 160;
constexpr std::uint32_t kUiBacklightTimeoutMs = 120000;
constexpr std::uint32_t kMidiMonitorActivityDecayMs = 180;
constexpr bool kUiEncoderDebugOverlay = false;
constexpr std::uint32_t kUiInjectedEventQueueDepth = 8;
constexpr std::uint8_t kUiButtonDebounceReads = 3;
constexpr std::uint16_t kUiButtonLongPressReads = 50;
constexpr std::size_t kMenuStackMaxDepth = 6;
constexpr std::size_t kLineBufferMaxLines = 256;
constexpr std::size_t kLineBufferLineCapacity = 21;
constexpr std::uint8_t kTftTextColumns = 20;
constexpr std::uint8_t kTftTextRows = 5;
constexpr std::uint8_t kTftFontWidth = 8;
constexpr std::uint8_t kTftFontHeight = 16;

}  // namespace midismith::main_board::app::config
