#if defined(UNIT_TESTS)

#include "app/ui/slide_animation.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>

namespace {

using midismith::main_board::app::ui::ComposeSlideRow;
using midismith::main_board::app::ui::EaseOutQuadraticSlideOffset;
using midismith::main_board::app::ui::kSlideAnimationSteps;
using midismith::main_board::app::ui::SlideDirection;

constexpr std::uint16_t kRowWidth = 8;

std::array<std::uint16_t, kRowWidth> MakeRow(std::uint16_t first_value) {
  std::array<std::uint16_t, kRowWidth> row{};
  for (std::uint16_t x = 0; x < kRowWidth; x++) {
    row[x] = static_cast<std::uint16_t>(first_value + x);
  }
  return row;
}

}  // namespace

TEST_CASE("The EaseOutQuadraticSlideOffset function") {
  SECTION("When stepping through the animation") {
    SECTION("Should grow monotonically and finish at the full width") {
      constexpr std::uint16_t kWidth = 160;
      std::uint16_t previous_offset = 0;

      for (std::size_t step = 0; step < kSlideAnimationSteps; step++) {
        const std::uint16_t offset = EaseOutQuadraticSlideOffset(kWidth, step);
        REQUIRE(offset >= previous_offset);
        previous_offset = offset;
      }

      REQUIRE(EaseOutQuadraticSlideOffset(kWidth, 0) > 0);
      REQUIRE(EaseOutQuadraticSlideOffset(kWidth, kSlideAnimationSteps - 1) == kWidth);
    }
  }
}

TEST_CASE("The ComposeSlideRow function") {
  SECTION("When sliding left") {
    SECTION("Should shift the previous row out and bring the next row in from the right") {
      const auto previous_row = MakeRow(0);
      const auto next_row = MakeRow(100);
      std::array<std::uint16_t, kRowWidth> destination{};

      ComposeSlideRow(destination.data(), previous_row.data(), next_row.data(), kRowWidth, 3,
                      SlideDirection::kLeft);

      REQUIRE(destination == std::array<std::uint16_t, kRowWidth>{3, 4, 5, 6, 7, 100, 101, 102});
    }
  }

  SECTION("When sliding right") {
    SECTION("Should bring the next row in from the left") {
      const auto previous_row = MakeRow(0);
      const auto next_row = MakeRow(100);
      std::array<std::uint16_t, kRowWidth> destination{};

      ComposeSlideRow(destination.data(), previous_row.data(), next_row.data(), kRowWidth, 3,
                      SlideDirection::kRight);

      REQUIRE(destination == std::array<std::uint16_t, kRowWidth>{105, 106, 107, 0, 1, 2, 3, 4});
    }
  }

  SECTION("When the offset reaches the full width") {
    SECTION("Should show only the next row") {
      const auto previous_row = MakeRow(0);
      const auto next_row = MakeRow(100);
      std::array<std::uint16_t, kRowWidth> destination{};

      ComposeSlideRow(destination.data(), previous_row.data(), next_row.data(), kRowWidth,
                      kRowWidth, SlideDirection::kLeft);

      REQUIRE(destination == next_row);
    }
  }
}

#endif
