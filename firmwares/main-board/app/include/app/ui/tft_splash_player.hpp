#pragma once

#include <cstdint>
#include <span>

#include "app/ui/splash_requirements.hpp"
#include "bsp-types/display/pixel_surface_requirements.hpp"
#include "os-types/delay_requirements.hpp"
#include "os-types/uptime_provider_requirements.hpp"

namespace midismith::main_board::app::ui {

class TftSplashPlayer final : public SplashRequirements {
 public:
  TftSplashPlayer(midismith::bsp::display::PixelSurfaceRequirements& surface, int band_row_count,
                  std::span<std::uint8_t> band_pixels, std::span<std::uint8_t> band_row_pixels,
                  std::span<std::uint16_t> band_row_colors, midismith::os::DelayRequirements& delay,
                  midismith::os::UptimeProviderRequirements& uptime, std::uint32_t frame_period_ms,
                  int saturation_percent) noexcept;

  void Play() noexcept override;

 private:
  void RenderFrameToDisplay(double time_seconds) noexcept;

  midismith::bsp::display::PixelSurfaceRequirements& surface_;
  int band_row_count_;
  std::span<std::uint8_t> band_pixels_;
  std::span<std::uint8_t> band_row_pixels_;
  std::span<std::uint16_t> band_row_colors_;
  midismith::os::DelayRequirements& delay_;
  midismith::os::UptimeProviderRequirements& uptime_;
  std::uint32_t frame_period_ms_;
  int saturation_percent_;
};

}  // namespace midismith::main_board::app::ui
