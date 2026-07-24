#if defined(UNIT_TESTS)

#include "splash/pixel_canvas.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

using midismith::splash::Color;
using midismith::splash::PixelCanvas;
using midismith::splash::Point;

namespace {

constexpr int kTestWidth = 4;
constexpr int kTestHeight = 4;
constexpr Color kWhite{255, 255, 255};

struct CanvasFixture {
  CanvasFixture()
      : buffer(PixelCanvas::SupersampledBufferBytes(kTestWidth, kTestHeight), 0),
        canvas(kTestWidth, kTestHeight, buffer) {}

  std::uint8_t SupersampledChannel(int pixel_x, int pixel_y, int channel) const {
    const std::size_t scaled_width =
        static_cast<std::size_t>(kTestWidth) * PixelCanvas::kSupersamplingScale;
    return buffer[(static_cast<std::size_t>(pixel_y) * scaled_width + pixel_x) * 3 + channel];
  }

  std::vector<std::uint8_t> buffer;
  PixelCanvas canvas;
};

}  // namespace

TEST_CASE("The PixelCanvas class") {
  SECTION("The BlendPixel() method") {
    SECTION("When blending with full opacity") {
      SECTION("Should overwrite the pixel with the color") {
        CanvasFixture fixture;

        fixture.canvas.BlendPixel(3, 2, {10, 20, 30}, 1.0);

        REQUIRE(fixture.SupersampledChannel(3, 2, 0) == 10);
        REQUIRE(fixture.SupersampledChannel(3, 2, 1) == 20);
        REQUIRE(fixture.SupersampledChannel(3, 2, 2) == 30);
      }
    }
    SECTION("When blending with half opacity over black") {
      SECTION("Should mix the color toward black") {
        CanvasFixture fixture;

        fixture.canvas.BlendPixel(0, 0, {100, 100, 100}, 0.5);

        REQUIRE(fixture.SupersampledChannel(0, 0, 0) == 50);
      }
    }
    SECTION("When the opacity is zero or the pixel is out of bounds") {
      SECTION("Should leave the buffer untouched") {
        CanvasFixture fixture;

        fixture.canvas.BlendPixel(1, 1, kWhite, 0.0);
        fixture.canvas.BlendPixel(-1, 0, kWhite, 1.0);
        fixture.canvas.BlendPixel(0, 1000, kWhite, 1.0);

        for (const std::uint8_t value : fixture.buffer) {
          REQUIRE(value == 0);
        }
      }
    }
  }

  SECTION("The DrawDisc() method") {
    SECTION("When drawing a disc at the canvas center") {
      SECTION("Should color the center and keep the corners black") {
        CanvasFixture fixture;

        fixture.canvas.DrawDisc(2.0, 2.0, 1.0, kWhite, 1.0);

        REQUIRE(fixture.SupersampledChannel(8, 8, 0) == 255);
        REQUIRE(fixture.SupersampledChannel(0, 0, 0) == 0);
        REQUIRE(fixture.SupersampledChannel(15, 15, 0) == 0);
      }
    }
  }

  SECTION("The FillPolygon() method") {
    SECTION("When filling an axis-aligned square") {
      SECTION("Should color the interior and keep the exterior black") {
        CanvasFixture fixture;
        const std::array<Point, 4> square{{{1.0, 1.0}, {3.0, 1.0}, {3.0, 3.0}, {1.0, 3.0}}};

        fixture.canvas.FillPolygon(square, kWhite, 1.0);

        REQUIRE(fixture.SupersampledChannel(8, 8, 0) == 255);
        REQUIRE(fixture.SupersampledChannel(1, 1, 0) == 0);
        REQUIRE(fixture.SupersampledChannel(14, 14, 0) == 0);
      }
    }
  }

  SECTION("The DrawPartialPolyline() method") {
    SECTION("When the fraction is zero") {
      SECTION("Should leave the buffer untouched") {
        CanvasFixture fixture;
        const std::array<Point, 3> polyline{{{0.0, 0.0}, {2.0, 2.0}, {4.0, 0.0}}};

        fixture.canvas.DrawPartialPolyline(polyline, 0.0, 1.0, kWhite, 1.0);

        for (const std::uint8_t value : fixture.buffer) {
          REQUIRE(value == 0);
        }
      }
    }
  }

  SECTION("The Downsample() method") {
    SECTION("When the supersampled buffer holds a uniform color") {
      SECTION("Should keep the color exactly") {
        CanvasFixture fixture;
        for (std::size_t offset = 0; offset < fixture.buffer.size(); offset += 3) {
          fixture.buffer[offset] = 120;
          fixture.buffer[offset + 1] = 130;
          fixture.buffer[offset + 2] = 140;
        }
        std::vector<std::uint8_t> output(PixelCanvas::OutputBufferBytes(kTestWidth, kTestHeight),
                                         0);

        fixture.canvas.Downsample(output);

        REQUIRE(output[0] == 120);
        REQUIRE(output[1] == 130);
        REQUIRE(output[2] == 140);
        REQUIRE(output[output.size() - 1] == 140);
      }
    }
  }

  SECTION("The DownsampleToRgb565() method") {
    SECTION("When the supersampled buffer holds a uniform color") {
      SECTION("Should pack the averaged color as RGB565") {
        CanvasFixture fixture;
        for (std::size_t offset = 0; offset < fixture.buffer.size(); offset += 3) {
          fixture.buffer[offset] = 120;
          fixture.buffer[offset + 1] = 130;
          fixture.buffer[offset + 2] = 140;
        }
        std::vector<std::uint16_t> output(static_cast<std::size_t>(kTestWidth) * kTestHeight, 0);

        fixture.canvas.DownsampleToRgb565(output);

        const std::uint16_t expected_color =
            static_cast<std::uint16_t>(((120 & 0xF8) << 8) | ((130 & 0xFC) << 3) | (140 >> 3));
        REQUIRE(output[0] == expected_color);
        REQUIRE(output[output.size() - 1] == expected_color);
      }
    }
  }

  SECTION("The band constructor") {
    SECTION("When the canvas covers a horizontal band") {
      SECTION("Should store band pixels relative to the band start and reject the rest") {
        std::vector<std::uint8_t> band_buffer(PixelCanvas::BandBufferBytes(kTestWidth, 2), 0);
        PixelCanvas band_canvas(kTestWidth, kTestHeight, 2, 2, band_buffer);

        band_canvas.BlendPixel(0, 7, kWhite, 1.0);
        band_canvas.BlendPixel(0, 8, kWhite, 1.0);

        REQUIRE(band_buffer[0] == 255);
        std::size_t colored_byte_count = 0;
        for (const std::uint8_t value : band_buffer) {
          if (value != 0) {
            ++colored_byte_count;
          }
        }
        REQUIRE(colored_byte_count == 3);
      }
    }
  }
}

#endif
