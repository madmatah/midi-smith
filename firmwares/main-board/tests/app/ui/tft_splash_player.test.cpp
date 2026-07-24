#if defined(UNIT_TESTS)

#include "app/ui/tft_splash_player.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

#include "app/config/ui.hpp"
#include "splash/animation.hpp"
#include "splash/pixel_canvas.hpp"

namespace {

using midismith::main_board::app::ui::TftSplashPlayer;

constexpr int kBandRows = midismith::main_board::app::config::kSplashBandRows;
constexpr std::uint32_t kFramePeriodMs = midismith::main_board::app::config::kSplashFramePeriodMs;

class CountingSurface final : public midismith::bsp::display::PixelSurfaceRequirements {
 public:
  void BlitRows(std::uint16_t y, std::uint16_t row_count, const std::uint8_t*) noexcept override {
    blits.push_back(BlitRecord{y, row_count});
  }

  std::uint16_t width() const noexcept override {
    return midismith::splash::kDisplayWidth;
  }

  std::uint16_t height() const noexcept override {
    return midismith::splash::kDisplayHeight;
  }

  struct BlitRecord {
    std::uint16_t y;
    std::uint16_t row_count;
  };

  std::vector<BlitRecord> blits;
};

class VirtualClock final : public midismith::os::DelayRequirements,
                           public midismith::os::UptimeProviderRequirements {
 public:
  void DelayMs(std::uint32_t milliseconds) noexcept override {
    now_ms += milliseconds;
    delay_count++;
  }

  std::uint32_t GetUptimeMs() const noexcept override {
    return now_ms;
  }

  std::uint32_t now_ms = 0;
  int delay_count = 0;
};

struct PlayerFixture {
  CountingSurface surface;
  VirtualClock clock;
  std::array<std::uint8_t, midismith::splash::PixelCanvas::BandBufferBytes(
                               midismith::splash::kDisplayWidth, kBandRows)>
      band_pixels{};
  std::array<std::uint8_t,
             static_cast<std::size_t>(midismith::splash::kDisplayWidth) * kBandRows * 3>
      band_row_pixels{};
  std::array<std::uint16_t, static_cast<std::size_t>(midismith::splash::kDisplayWidth) * kBandRows>
      band_row_colors{};
  TftSplashPlayer player{surface,
                         kBandRows,
                         band_pixels,
                         band_row_pixels,
                         band_row_colors,
                         clock,
                         clock,
                         kFramePeriodMs,
                         midismith::main_board::app::config::kSplashSaturationPercent};
};

constexpr int kBandsPerFrame = midismith::splash::kDisplayHeight / kBandRows;

}  // namespace

TEST_CASE("The TftSplashPlayer class") {
  PlayerFixture fixture;

  SECTION("The Play() method") {
    SECTION("When the animation runs to completion") {
      SECTION("Should cover the whole panel on every frame") {
        fixture.player.Play();

        REQUIRE(fixture.surface.blits.size() % kBandsPerFrame == 0);
        for (std::size_t index = 0; index < fixture.surface.blits.size(); index++) {
          const auto& blit = fixture.surface.blits[index];
          REQUIRE(blit.row_count == kBandRows);
          REQUIRE(blit.y == (index % kBandsPerFrame) * kBandRows);
          REQUIRE(blit.y + blit.row_count <= midismith::splash::kDisplayHeight);
        }
      }

      SECTION("Should advance the clock past the animation duration") {
        const auto duration_ms =
            static_cast<std::uint32_t>(midismith::splash::kAnimationDurationSeconds * 1000.0);

        fixture.player.Play();

        REQUIRE(fixture.clock.now_ms >= duration_ms);
      }

      SECTION("Should pace the frames instead of spinning") {
        fixture.player.Play();

        const auto duration_ms =
            static_cast<std::uint32_t>(midismith::splash::kAnimationDurationSeconds * 1000.0);
        const int expected_frames = static_cast<int>(duration_ms / kFramePeriodMs);
        REQUIRE(fixture.clock.delay_count >= expected_frames - 1);
      }

      SECTION("Should draw one final frame after the loop ends") {
        fixture.player.Play();

        const int rendered_frames = static_cast<int>(fixture.surface.blits.size()) / kBandsPerFrame;
        const auto duration_ms =
            static_cast<std::uint32_t>(midismith::splash::kAnimationDurationSeconds * 1000.0);
        REQUIRE(rendered_frames == static_cast<int>(duration_ms / kFramePeriodMs) + 1);
      }
    }
  }
}

#endif
