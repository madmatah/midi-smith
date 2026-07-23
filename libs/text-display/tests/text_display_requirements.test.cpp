#if defined(UNIT_TESTS)

#include "text-display/text_display_requirements.hpp"

#include <array>
#include <catch2/catch_test_macros.hpp>
#include <string_view>

namespace {

class RecordingTextDisplay final : public midismith::text_display::TextDisplayRequirements {
 public:
  std::uint8_t columns() const noexcept override {
    return columns_;
  }
  std::uint8_t rows() const noexcept override {
    return rows_;
  }

  void Clear() noexcept override {
    clear_count_++;
  }

  void DrawText(std::uint8_t row, std::uint8_t column, std::string_view text,
                midismith::text_display::CellAttribute attribute) noexcept override {
    last_row_ = row;
    last_column_ = column;
    last_text_ = text;
    last_attribute_ = attribute;
  }

  void FillRow(std::uint8_t row,
               midismith::text_display::CellAttribute attribute) noexcept override {
    filled_row_ = row;
    filled_attribute_ = attribute;
  }

  void Flush() noexcept override {
    flush_count_++;
  }

  static constexpr std::uint8_t columns_ = 16;
  static constexpr std::uint8_t rows_ = 10;
  std::uint8_t clear_count_ = 0;
  std::uint8_t flush_count_ = 0;
  std::uint8_t last_row_ = 0;
  std::uint8_t last_column_ = 0;
  std::string_view last_text_;
  midismith::text_display::CellAttribute last_attribute_ =
      midismith::text_display::CellAttribute::kNormal;
  std::uint8_t filled_row_ = 0;
  midismith::text_display::CellAttribute filled_attribute_ =
      midismith::text_display::CellAttribute::kNormal;
};

}  // namespace

TEST_CASE("The TextDisplayRequirements interface") {
  SECTION("The DrawText() method") {
    SECTION("When a concrete display records a highlighted text cell") {
      SECTION("Should preserve the text position and attribute") {
        RecordingTextDisplay display;

        display.DrawText(2, 4, "Config", midismith::text_display::CellAttribute::kHighlight);

        REQUIRE(display.columns() == 16);
        REQUIRE(display.rows() == 10);
        REQUIRE(display.last_row_ == 2);
        REQUIRE(display.last_column_ == 4);
        REQUIRE(display.last_text_ == "Config");
        REQUIRE(display.last_attribute_ == midismith::text_display::CellAttribute::kHighlight);
      }
    }
  }

  SECTION("The FillRow() method") {
    SECTION("When a concrete display records a dim row fill") {
      SECTION("Should preserve the row and attribute") {
        RecordingTextDisplay display;

        display.FillRow(3, midismith::text_display::CellAttribute::kDim);

        REQUIRE(display.filled_row_ == 3);
        REQUIRE(display.filled_attribute_ == midismith::text_display::CellAttribute::kDim);
      }
    }
  }
}

#endif
