#pragma once

#include <cstdint>
#include <span>

#include "app/ui/splash_requirements.hpp"
#include "bsp/tft_display.hpp"

namespace midismith::main_board::app::ui {

class TftSplashPlayer final : public SplashRequirements {
 public:
  TftSplashPlayer(midismith::main_board::bsp::TftDisplay& display, int band_row_count,
                  std::span<std::uint8_t> band_pixels, std::span<std::uint8_t> band_row_pixels,
                  std::span<std::uint16_t> band_row_colors, std::uint32_t frame_period_ms,
                  int saturation_percent) noexcept;

  void Play() noexcept override;

 private:
  void RenderFrameToDisplay(double time_seconds) noexcept;

  midismith::main_board::bsp::TftDisplay& display_;
  int band_row_count_;
  std::span<std::uint8_t> band_pixels_;
  std::span<std::uint8_t> band_row_pixels_;
  std::span<std::uint16_t> band_row_colors_;
  std::uint32_t frame_period_ms_;
  int saturation_percent_;
};

}  // namespace midismith::main_board::app::ui
