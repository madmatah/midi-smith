#include "app/ui/tft_splash_player.hpp"

#include <algorithm>

#include "os/clock.hpp"
#include "splash/animation.hpp"
#include "splash/pixel_canvas.hpp"

namespace midismith::main_board::app::ui {

namespace {

constexpr int ClampChannel(int value) noexcept {
  if (value < 0) {
    return 0;
  }
  if (value > 255) {
    return 255;
  }
  return value;
}

constexpr std::uint16_t PackPanelPixel(int red, int green, int blue) noexcept {
  const auto rgb565 =
      static_cast<std::uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
  return static_cast<std::uint16_t>((rgb565 << 8) | (rgb565 >> 8));
}

constexpr int kVibranceChromaFloor = 24;
constexpr int kVibranceChromaSpan = 40;

constexpr std::uint16_t PackVibrantPanelPixel(std::uint8_t red, std::uint8_t green,
                                              std::uint8_t blue, int saturation_percent) noexcept {
  const int maximum_channel =
      std::max(static_cast<int>(red), std::max(static_cast<int>(green), static_cast<int>(blue)));
  const int minimum_channel =
      std::min(static_cast<int>(red), std::min(static_cast<int>(green), static_cast<int>(blue)));
  const int chroma = maximum_channel - minimum_channel;
  if (chroma <= kVibranceChromaFloor || saturation_percent == 100) {
    return PackPanelPixel(red, green, blue);
  }
  const int chroma_weight = std::min(chroma - kVibranceChromaFloor, kVibranceChromaSpan);
  const int effective_percent =
      100 + ((saturation_percent - 100) * chroma_weight) / kVibranceChromaSpan;
  const int luma = (red * 77 + green * 150 + blue * 29) >> 8;
  const int boosted_red = ClampChannel(luma + ((red - luma) * effective_percent) / 100);
  const int boosted_green = ClampChannel(luma + ((green - luma) * effective_percent) / 100);
  const int boosted_blue = ClampChannel(luma + ((blue - luma) * effective_percent) / 100);
  return PackPanelPixel(boosted_red, boosted_green, boosted_blue);
}

}  // namespace

TftSplashPlayer::TftSplashPlayer(midismith::bsp::display::PixelSurfaceRequirements& surface,
                                 int band_row_count, std::span<std::uint8_t> band_pixels,
                                 std::span<std::uint8_t> band_row_pixels,
                                 std::span<std::uint16_t> band_row_colors,
                                 std::uint32_t frame_period_ms, int saturation_percent) noexcept
    : surface_(surface),
      band_row_count_(band_row_count),
      band_pixels_(band_pixels),
      band_row_pixels_(band_row_pixels),
      band_row_colors_(band_row_colors),
      frame_period_ms_(frame_period_ms),
      saturation_percent_(saturation_percent) {}

void TftSplashPlayer::Play() noexcept {
  const auto duration_ms =
      static_cast<std::uint32_t>(midismith::splash::kAnimationDurationSeconds * 1000.0);
  const std::uint32_t start_ms = midismith::os::Clock::now_ms();
  std::uint32_t elapsed_ms = 0;
  while (elapsed_ms < duration_ms) {
    RenderFrameToDisplay(static_cast<double>(elapsed_ms) / 1000.0);
    const std::uint32_t frame_elapsed_ms = midismith::os::Clock::now_ms() - start_ms;
    const std::uint32_t next_frame_ms = elapsed_ms + frame_period_ms_;
    if (frame_elapsed_ms < next_frame_ms) {
      midismith::os::Clock::delay_ms(next_frame_ms - frame_elapsed_ms);
    }
    elapsed_ms = midismith::os::Clock::now_ms() - start_ms;
  }
  RenderFrameToDisplay(static_cast<double>(duration_ms) / 1000.0);
}

void TftSplashPlayer::RenderFrameToDisplay(double time_seconds) noexcept {
  for (int band_first_row = 0; band_first_row < midismith::splash::kDisplayHeight;
       band_first_row += band_row_count_) {
    midismith::splash::PixelCanvas canvas(midismith::splash::kDisplayWidth,
                                          midismith::splash::kDisplayHeight, band_first_row,
                                          band_row_count_, band_pixels_);
    midismith::splash::RenderFrame(time_seconds, canvas);
    canvas.Downsample(band_row_pixels_);
    for (std::size_t pixel_index = 0; pixel_index < band_row_colors_.size(); ++pixel_index) {
      const std::size_t source_offset = pixel_index * 3;
      band_row_colors_[pixel_index] = PackVibrantPanelPixel(
          band_row_pixels_[source_offset], band_row_pixels_[source_offset + 1],
          band_row_pixels_[source_offset + 2], saturation_percent_);
    }
    surface_.BlitRows(static_cast<std::uint16_t>(band_first_row),
                      static_cast<std::uint16_t>(band_row_count_),
                      reinterpret_cast<const std::uint8_t*>(band_row_colors_.data()));
  }
}

}  // namespace midismith::main_board::app::ui
