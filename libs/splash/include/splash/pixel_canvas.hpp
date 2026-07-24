#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "splash/geometry.hpp"
#include "splash/palette.hpp"

namespace midismith::splash {

class PixelCanvas {
 public:
  static constexpr int kSupersamplingScale = 4;
  static constexpr std::size_t kMaxPolygonPoints = 48;

  static constexpr std::size_t SupersampledBufferBytes(int width, int height) {
    return static_cast<std::size_t>(width) * kSupersamplingScale *
           static_cast<std::size_t>(height) * kSupersamplingScale * 3;
  }

  static constexpr std::size_t BandBufferBytes(int width, int band_row_count) {
    return SupersampledBufferBytes(width, band_row_count);
  }

  static constexpr std::size_t OutputBufferBytes(int width, int height) {
    return static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3;
  }

  PixelCanvas(int width, int height, std::span<std::uint8_t> supersampled_pixels) noexcept;
  PixelCanvas(int width, int height, int band_first_row, int band_row_count,
              std::span<std::uint8_t> band_pixels) noexcept;

  void Clear() noexcept;
  void BlendPixel(int pixel_x, int pixel_y, Color color, double opacity) noexcept;
  void DrawDisc(double center_x, double center_y, double radius, Color color,
                double opacity) noexcept;
  void DrawEllipse(double center_x, double center_y, double radius_x, double radius_y,
                   double tilt_radians, Color color, double opacity) noexcept;
  void DrawLine(double start_x, double start_y, double end_x, double end_y, double width,
                Color color, double opacity) noexcept;
  void DrawPolyline(std::span<const Point> points, double width, Color color,
                    double opacity) noexcept;
  void DrawPartialPolyline(std::span<const Point> points, double fraction, double width,
                           Color color, double opacity) noexcept;
  void FillPolygon(std::span<const Point> points, Color color, double opacity) noexcept;
  void Downsample(std::span<std::uint8_t> output_pixels) const noexcept;
  void DownsampleToRgb565(std::span<std::uint16_t> output_pixels) const noexcept;

  int width() const noexcept {
    return width_;
  }
  int height() const noexcept {
    return height_;
  }
  int band_first_row() const noexcept {
    return band_first_row_;
  }
  int band_row_count() const noexcept {
    return band_row_count_;
  }

 private:
  int FirstBandRow(int minimum_y) const noexcept;
  int LastBandRow(int maximum_y) const noexcept;

  int width_;
  int height_;
  int scaled_width_;
  int band_first_row_;
  int band_row_count_;
  int band_first_scaled_row_;
  int band_end_scaled_row_;
  std::span<std::uint8_t> pixels_;
};

}  // namespace midismith::splash
