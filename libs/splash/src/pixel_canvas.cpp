#include "splash/pixel_canvas.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "splash/easing.hpp"

namespace midismith::splash {

PixelCanvas::PixelCanvas(int width, int height,
                         std::span<std::uint8_t> supersampled_pixels) noexcept
    : PixelCanvas(width, height, 0, height, supersampled_pixels) {}

PixelCanvas::PixelCanvas(int width, int height, int band_first_row, int band_row_count,
                         std::span<std::uint8_t> band_pixels) noexcept
    : width_(width),
      height_(height),
      scaled_width_(width * kSupersamplingScale),
      band_first_row_(band_first_row),
      band_row_count_(band_row_count),
      band_first_scaled_row_(band_first_row * kSupersamplingScale),
      band_end_scaled_row_((band_first_row + band_row_count) * kSupersamplingScale),
      pixels_(band_pixels) {}

int PixelCanvas::FirstBandRow(int minimum_y) const noexcept {
  return std::max(minimum_y, band_first_scaled_row_);
}

int PixelCanvas::LastBandRow(int maximum_y) const noexcept {
  return std::min(maximum_y, band_end_scaled_row_ - 1);
}

void PixelCanvas::Clear() noexcept {
  std::fill(pixels_.begin(), pixels_.end(), std::uint8_t{0});
}

void PixelCanvas::BlendPixel(int pixel_x, int pixel_y, Color color, double opacity) noexcept {
  if (pixel_x < 0 || pixel_x >= scaled_width_ || pixel_y < band_first_scaled_row_ ||
      pixel_y >= band_end_scaled_row_ || opacity <= 0.0) {
    return;
  }
  const double clamped_opacity = Clamp(opacity, 0.0, 1.0);
  const std::size_t offset = (static_cast<std::size_t>(pixel_y - band_first_scaled_row_) *
                                  static_cast<std::size_t>(scaled_width_) +
                              static_cast<std::size_t>(pixel_x)) *
                             3;
  const double inverse_opacity = 1.0 - clamped_opacity;
  pixels_[offset] = static_cast<std::uint8_t>(
      std::nearbyint(pixels_[offset] * inverse_opacity + color.red * clamped_opacity));
  pixels_[offset + 1] = static_cast<std::uint8_t>(
      std::nearbyint(pixels_[offset + 1] * inverse_opacity + color.green * clamped_opacity));
  pixels_[offset + 2] = static_cast<std::uint8_t>(
      std::nearbyint(pixels_[offset + 2] * inverse_opacity + color.blue * clamped_opacity));
}

void PixelCanvas::DrawDisc(double center_x, double center_y, double radius, Color color,
                           double opacity) noexcept {
  const double scaled_center_x = center_x * kSupersamplingScale;
  const double scaled_center_y = center_y * kSupersamplingScale;
  const double scaled_radius = radius * kSupersamplingScale;
  const int minimum_x = static_cast<int>(std::floor(scaled_center_x - scaled_radius));
  const int maximum_x = static_cast<int>(std::ceil(scaled_center_x + scaled_radius));
  const int minimum_y = FirstBandRow(static_cast<int>(std::floor(scaled_center_y - scaled_radius)));
  const int maximum_y = LastBandRow(static_cast<int>(std::ceil(scaled_center_y + scaled_radius)));
  const double radius_squared = scaled_radius * scaled_radius;
  for (int pixel_y = minimum_y; pixel_y <= maximum_y; ++pixel_y) {
    const double distance_y = pixel_y + 0.5 - scaled_center_y;
    for (int pixel_x = minimum_x; pixel_x <= maximum_x; ++pixel_x) {
      const double distance_x = pixel_x + 0.5 - scaled_center_x;
      if (distance_x * distance_x + distance_y * distance_y <= radius_squared) {
        BlendPixel(pixel_x, pixel_y, color, opacity);
      }
    }
  }
}

void PixelCanvas::DrawEllipse(double center_x, double center_y, double radius_x, double radius_y,
                              double tilt_radians, Color color, double opacity) noexcept {
  if (radius_x <= 0.0 || radius_y <= 0.0) {
    return;
  }
  const double scaled_center_x = center_x * kSupersamplingScale;
  const double scaled_center_y = center_y * kSupersamplingScale;
  const double scaled_radius_x = radius_x * kSupersamplingScale;
  const double scaled_radius_y = radius_y * kSupersamplingScale;
  const double extent = std::max(scaled_radius_x, scaled_radius_y);
  const int minimum_x = static_cast<int>(std::floor(scaled_center_x - extent));
  const int maximum_x = static_cast<int>(std::ceil(scaled_center_x + extent));
  const int minimum_y = FirstBandRow(static_cast<int>(std::floor(scaled_center_y - extent)));
  const int maximum_y = LastBandRow(static_cast<int>(std::ceil(scaled_center_y + extent)));
  const double cosine = std::cos(tilt_radians);
  const double sine = std::sin(tilt_radians);
  for (int pixel_y = minimum_y; pixel_y <= maximum_y; ++pixel_y) {
    const double distance_y = pixel_y + 0.5 - scaled_center_y;
    for (int pixel_x = minimum_x; pixel_x <= maximum_x; ++pixel_x) {
      const double distance_x = pixel_x + 0.5 - scaled_center_x;
      const double local_x = cosine * distance_x + sine * distance_y;
      const double local_y = -sine * distance_x + cosine * distance_y;
      const double normalized_x = local_x / scaled_radius_x;
      const double normalized_y = local_y / scaled_radius_y;
      if (normalized_x * normalized_x + normalized_y * normalized_y <= 1.0) {
        BlendPixel(pixel_x, pixel_y, color, opacity);
      }
    }
  }
}

void PixelCanvas::DrawLine(double start_x, double start_y, double end_x, double end_y, double width,
                           Color color, double opacity) noexcept {
  const double scaled_start_x = start_x * kSupersamplingScale;
  const double scaled_start_y = start_y * kSupersamplingScale;
  const double scaled_end_x = end_x * kSupersamplingScale;
  const double scaled_end_y = end_y * kSupersamplingScale;
  const double scaled_radius = width * kSupersamplingScale / 2.0;
  const int minimum_x =
      static_cast<int>(std::floor(std::min(scaled_start_x, scaled_end_x) - scaled_radius));
  const int maximum_x =
      static_cast<int>(std::ceil(std::max(scaled_start_x, scaled_end_x) + scaled_radius));
  const int minimum_y = FirstBandRow(
      static_cast<int>(std::floor(std::min(scaled_start_y, scaled_end_y) - scaled_radius)));
  const int maximum_y = LastBandRow(
      static_cast<int>(std::ceil(std::max(scaled_start_y, scaled_end_y) + scaled_radius)));
  const double direction_x = scaled_end_x - scaled_start_x;
  const double direction_y = scaled_end_y - scaled_start_y;
  const double length_squared = direction_x * direction_x + direction_y * direction_y;
  const double radius_squared = scaled_radius * scaled_radius;
  for (int pixel_y = minimum_y; pixel_y <= maximum_y; ++pixel_y) {
    const double sample_y = pixel_y + 0.5;
    for (int pixel_x = minimum_x; pixel_x <= maximum_x; ++pixel_x) {
      const double sample_x = pixel_x + 0.5;
      double projection = 0.0;
      if (length_squared != 0.0) {
        projection = ((sample_x - scaled_start_x) * direction_x +
                      (sample_y - scaled_start_y) * direction_y) /
                     length_squared;
      }
      projection = Clamp(projection, 0.0, 1.0);
      const double nearest_x = scaled_start_x + projection * direction_x;
      const double nearest_y = scaled_start_y + projection * direction_y;
      const double distance_x = sample_x - nearest_x;
      const double distance_y = sample_y - nearest_y;
      if (distance_x * distance_x + distance_y * distance_y <= radius_squared) {
        BlendPixel(pixel_x, pixel_y, color, opacity);
      }
    }
  }
}

void PixelCanvas::DrawPolyline(std::span<const Point> points, double width, Color color,
                               double opacity) noexcept {
  if (points.size() < 2) {
    return;
  }
  for (std::size_t point_index = 0; point_index + 1 < points.size(); ++point_index) {
    DrawLine(points[point_index].x, points[point_index].y, points[point_index + 1].x,
             points[point_index + 1].y, width, color, opacity);
  }
}

void PixelCanvas::DrawPartialPolyline(std::span<const Point> points, double fraction, double width,
                                      Color color, double opacity) noexcept {
  const double clamped_fraction = Clamp(fraction, 0.0, 1.0);
  if (clamped_fraction <= 0.0 || points.size() < 2) {
    return;
  }
  const auto last_index =
      std::max(std::size_t{1}, static_cast<std::size_t>(std::nearbyint(
                                   clamped_fraction * static_cast<double>(points.size() - 1))));
  DrawPolyline(points.first(last_index + 1), width, color, opacity);
}

void PixelCanvas::FillPolygon(std::span<const Point> points, Color color, double opacity) noexcept {
  if (points.size() < 3 || points.size() > kMaxPolygonPoints) {
    return;
  }
  std::array<Point, kMaxPolygonPoints> scaled_points{};
  for (std::size_t point_index = 0; point_index < points.size(); ++point_index) {
    scaled_points[point_index] = {points[point_index].x * kSupersamplingScale,
                                  points[point_index].y * kSupersamplingScale};
  }
  double lowest_y = scaled_points[0].y;
  double highest_y = scaled_points[0].y;
  for (std::size_t point_index = 1; point_index < points.size(); ++point_index) {
    lowest_y = std::min(lowest_y, scaled_points[point_index].y);
    highest_y = std::max(highest_y, scaled_points[point_index].y);
  }
  const int minimum_y = FirstBandRow(static_cast<int>(std::floor(lowest_y)));
  const int maximum_y = LastBandRow(static_cast<int>(std::ceil(highest_y)));
  for (int pixel_y = minimum_y; pixel_y <= maximum_y; ++pixel_y) {
    const double scan_y = pixel_y + 0.5;
    std::array<double, kMaxPolygonPoints> intersections{};
    std::size_t intersection_count = 0;
    for (std::size_t point_index = 0; point_index < points.size(); ++point_index) {
      const Point first_point = scaled_points[point_index];
      const Point second_point = scaled_points[(point_index + 1) % points.size()];
      const bool crosses_downward = first_point.y <= scan_y && scan_y < second_point.y;
      const bool crosses_upward = second_point.y <= scan_y && scan_y < first_point.y;
      if (crosses_downward || crosses_upward) {
        const double interpolation = (scan_y - first_point.y) / (second_point.y - first_point.y);
        intersections[intersection_count] =
            first_point.x + interpolation * (second_point.x - first_point.x);
        ++intersection_count;
      }
    }
    std::sort(intersections.begin(), intersections.begin() + intersection_count);
    for (std::size_t intersection_index = 0; intersection_index + 1 < intersection_count;
         intersection_index += 2) {
      const int minimum_x =
          std::max(0, static_cast<int>(std::ceil(intersections[intersection_index] - 0.5)));
      const int maximum_x =
          std::min(scaled_width_ - 1,
                   static_cast<int>(std::floor(intersections[intersection_index + 1] - 0.5)));
      for (int pixel_x = minimum_x; pixel_x <= maximum_x; ++pixel_x) {
        BlendPixel(pixel_x, pixel_y, color, opacity);
      }
    }
  }
}

void PixelCanvas::Downsample(std::span<std::uint8_t> output_pixels) const noexcept {
  constexpr int kSampleCount = kSupersamplingScale * kSupersamplingScale;
  for (int band_y = 0; band_y < band_row_count_; ++band_y) {
    for (int output_x = 0; output_x < width_; ++output_x) {
      int red_sum = 0;
      int green_sum = 0;
      int blue_sum = 0;
      for (int sample_y = 0; sample_y < kSupersamplingScale; ++sample_y) {
        const int source_y = band_y * kSupersamplingScale + sample_y;
        for (int sample_x = 0; sample_x < kSupersamplingScale; ++sample_x) {
          const int source_x = output_x * kSupersamplingScale + sample_x;
          const std::size_t source_offset =
              (static_cast<std::size_t>(source_y) * static_cast<std::size_t>(scaled_width_) +
               static_cast<std::size_t>(source_x)) *
              3;
          red_sum += pixels_[source_offset];
          green_sum += pixels_[source_offset + 1];
          blue_sum += pixels_[source_offset + 2];
        }
      }
      const std::size_t output_offset =
          (static_cast<std::size_t>(band_y) * static_cast<std::size_t>(width_) +
           static_cast<std::size_t>(output_x)) *
          3;
      output_pixels[output_offset] = static_cast<std::uint8_t>(red_sum / kSampleCount);
      output_pixels[output_offset + 1] = static_cast<std::uint8_t>(green_sum / kSampleCount);
      output_pixels[output_offset + 2] = static_cast<std::uint8_t>(blue_sum / kSampleCount);
    }
  }
}

void PixelCanvas::DownsampleToRgb565(std::span<std::uint16_t> output_pixels) const noexcept {
  constexpr int kSampleCount = kSupersamplingScale * kSupersamplingScale;
  for (int band_y = 0; band_y < band_row_count_; ++band_y) {
    for (int output_x = 0; output_x < width_; ++output_x) {
      int red_sum = 0;
      int green_sum = 0;
      int blue_sum = 0;
      for (int sample_y = 0; sample_y < kSupersamplingScale; ++sample_y) {
        const int source_y = band_y * kSupersamplingScale + sample_y;
        for (int sample_x = 0; sample_x < kSupersamplingScale; ++sample_x) {
          const int source_x = output_x * kSupersamplingScale + sample_x;
          const std::size_t source_offset =
              (static_cast<std::size_t>(source_y) * static_cast<std::size_t>(scaled_width_) +
               static_cast<std::size_t>(source_x)) *
              3;
          red_sum += pixels_[source_offset];
          green_sum += pixels_[source_offset + 1];
          blue_sum += pixels_[source_offset + 2];
        }
      }
      const int red = red_sum / kSampleCount;
      const int green = green_sum / kSampleCount;
      const int blue = blue_sum / kSampleCount;
      output_pixels[static_cast<std::size_t>(band_y) * static_cast<std::size_t>(width_) +
                    static_cast<std::size_t>(output_x)] =
          static_cast<std::uint16_t>(((red & 0xF8) << 8) | ((green & 0xFC) << 3) | (blue >> 3));
    }
  }
}

}  // namespace midismith::splash
