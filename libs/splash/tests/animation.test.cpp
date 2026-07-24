#if defined(UNIT_TESTS)

#include "splash/animation.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <span>
#include <string>
#include <vector>

using midismith::splash::kAnimationDurationSeconds;
using midismith::splash::kDisplayHeight;
using midismith::splash::kDisplayWidth;
using midismith::splash::PixelCanvas;
using midismith::splash::RenderFrame;

namespace {

constexpr std::array<double, 7> kGoldenFrameTimesSeconds{0.42, 0.66, 1.32, 1.92, 2.20, 2.85, 3.50};
constexpr int kMaxChannelDifference = 24;
constexpr double kMaxMeanChannelDifference = 0.05;
constexpr double kMinExactChannelRatio = 0.995;

struct PpmImage {
  int width = 0;
  int height = 0;
  std::vector<std::uint8_t> pixels;
};

std::optional<PpmImage> LoadPpm(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) {
    return std::nullopt;
  }
  std::string magic;
  int width = 0;
  int height = 0;
  int maximum_value = 0;
  input >> magic >> width >> height >> maximum_value;
  if (magic != "P6" || maximum_value != 255 || width <= 0 || height <= 0) {
    return std::nullopt;
  }
  input.get();
  PpmImage image{width, height,
                 std::vector<std::uint8_t>(static_cast<std::size_t>(width) * height * 3)};
  input.read(reinterpret_cast<char*>(image.pixels.data()),
             static_cast<std::streamsize>(image.pixels.size()));
  if (!input) {
    return std::nullopt;
  }
  return image;
}

std::string GoldenFramePath(double frame_time_seconds) {
  const long milliseconds = std::lround(frame_time_seconds * 1000.0);
  std::array<char, 32> file_name{};
  std::snprintf(file_name.data(), file_name.size(), "frame_%04ldms.ppm", milliseconds);
  return std::string(SPLASH_GOLDEN_DIRECTORY) + "/" + file_name.data();
}

struct FrameComparison {
  int max_difference = 0;
  double mean_difference = 0.0;
  double near_exact_ratio = 0.0;
};

FrameComparison CompareFrames(const std::vector<std::uint8_t>& rendered,
                              const std::vector<std::uint8_t>& reference) {
  FrameComparison comparison;
  long difference_sum = 0;
  std::size_t near_exact_count = 0;
  for (std::size_t offset = 0; offset < rendered.size(); ++offset) {
    const int difference =
        std::abs(static_cast<int>(rendered[offset]) - static_cast<int>(reference[offset]));
    comparison.max_difference = std::max(comparison.max_difference, difference);
    difference_sum += difference;
    if (difference <= 1) {
      ++near_exact_count;
    }
  }
  comparison.mean_difference =
      static_cast<double>(difference_sum) / static_cast<double>(rendered.size());
  comparison.near_exact_ratio =
      static_cast<double>(near_exact_count) / static_cast<double>(rendered.size());
  return comparison;
}

}  // namespace

TEST_CASE("The splash animation") {
  SECTION("The RenderFrame() function") {
    SECTION("When rendering the golden reference times") {
      SECTION("Should reproduce the Python reference frames") {
        std::vector<std::uint8_t> supersampled(
            PixelCanvas::SupersampledBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        std::vector<std::uint8_t> rendered(
            PixelCanvas::OutputBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        PixelCanvas canvas(kDisplayWidth, kDisplayHeight, supersampled);

        for (const double frame_time_seconds : kGoldenFrameTimesSeconds) {
          const std::string golden_path = GoldenFramePath(frame_time_seconds);
          CAPTURE(golden_path);

          const std::optional<PpmImage> reference = LoadPpm(golden_path);
          REQUIRE(reference.has_value());
          REQUIRE(reference->width == kDisplayWidth);
          REQUIRE(reference->height == kDisplayHeight);

          RenderFrame(frame_time_seconds, canvas);
          canvas.Downsample(rendered);

          const FrameComparison comparison = CompareFrames(rendered, reference->pixels);
          CAPTURE(comparison.max_difference, comparison.mean_difference,
                  comparison.near_exact_ratio);
          REQUIRE(comparison.max_difference <= kMaxChannelDifference);
          REQUIRE(comparison.mean_difference <= kMaxMeanChannelDifference);
          REQUIRE(comparison.near_exact_ratio >= kMinExactChannelRatio);
        }
      }
    }
    SECTION("When rendering past the animation duration") {
      SECTION("Should hold the final identity frame") {
        std::vector<std::uint8_t> supersampled(
            PixelCanvas::SupersampledBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        std::vector<std::uint8_t> last_frame(
            PixelCanvas::OutputBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        std::vector<std::uint8_t> held_frame(
            PixelCanvas::OutputBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        PixelCanvas canvas(kDisplayWidth, kDisplayHeight, supersampled);

        RenderFrame(kAnimationDurationSeconds, canvas);
        canvas.Downsample(last_frame);
        RenderFrame(kAnimationDurationSeconds + 5.0, canvas);
        canvas.Downsample(held_frame);

        REQUIRE(last_frame == held_frame);
      }
    }
    SECTION("When rendering the frame in horizontal bands") {
      SECTION("Should match the full-frame rendering exactly") {
        constexpr int kBandRowCount = 8;
        std::vector<std::uint8_t> supersampled(
            PixelCanvas::SupersampledBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        std::vector<std::uint8_t> full_frame(
            PixelCanvas::OutputBufferBytes(kDisplayWidth, kDisplayHeight), 0);
        PixelCanvas full_canvas(kDisplayWidth, kDisplayHeight, supersampled);
        std::vector<std::uint8_t> band_pixels(
            PixelCanvas::BandBufferBytes(kDisplayWidth, kBandRowCount), 0);
        std::vector<std::uint8_t> banded_frame(
            PixelCanvas::OutputBufferBytes(kDisplayWidth, kDisplayHeight), 0);

        for (const double frame_time_seconds : kGoldenFrameTimesSeconds) {
          CAPTURE(frame_time_seconds);
          RenderFrame(frame_time_seconds, full_canvas);
          full_canvas.Downsample(full_frame);
          for (int band_first_row = 0; band_first_row < kDisplayHeight;
               band_first_row += kBandRowCount) {
            PixelCanvas band_canvas(kDisplayWidth, kDisplayHeight, band_first_row, kBandRowCount,
                                    band_pixels);
            RenderFrame(frame_time_seconds, band_canvas);
            const std::span<std::uint8_t> band_output(
                banded_frame.data() + static_cast<std::size_t>(band_first_row) * kDisplayWidth * 3,
                static_cast<std::size_t>(kBandRowCount) * kDisplayWidth * 3);
            band_canvas.Downsample(band_output);
          }
          REQUIRE(banded_frame == full_frame);
        }
      }
    }
  }
}

#endif
