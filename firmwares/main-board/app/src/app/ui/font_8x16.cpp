#include "app/ui/font_8x16.hpp"

#include <array>

namespace midismith::main_board::app::ui {

namespace {

constexpr std::uint64_t Pattern(std::uint8_t row0, std::uint8_t row1, std::uint8_t row2,
                                std::uint8_t row3, std::uint8_t row4, std::uint8_t row5,
                                std::uint8_t row6) noexcept {
  return (static_cast<std::uint64_t>(row0) << 30) | (static_cast<std::uint64_t>(row1) << 25) |
         (static_cast<std::uint64_t>(row2) << 20) | (static_cast<std::uint64_t>(row3) << 15) |
         (static_cast<std::uint64_t>(row4) << 10) | (static_cast<std::uint64_t>(row5) << 5) | row6;
}

constexpr char ToUpper(char character) noexcept {
  if (character >= 'a' && character <= 'z') {
    return static_cast<char>(character - 'a' + 'A');
  }
  return character;
}

constexpr std::uint64_t GlyphPattern(char character) noexcept {
  switch (ToUpper(character)) {
    case ' ':
      return 0;
    case 'A':
      return Pattern(0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001);
    case 'B':
      return Pattern(0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110);
    case 'C':
      return Pattern(0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110);
    case 'D':
      return Pattern(0b11110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11110);
    case 'E':
      return Pattern(0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111);
    case 'F':
      return Pattern(0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000);
    case 'G':
      return Pattern(0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110);
    case 'H':
      return Pattern(0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001);
    case 'I':
      return Pattern(0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111);
    case 'J':
      return Pattern(0b00111, 0b00010, 0b00010, 0b00010, 0b10010, 0b10010, 0b01100);
    case 'K':
      return Pattern(0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001);
    case 'L':
      return Pattern(0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111);
    case 'M':
      return Pattern(0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001);
    case 'N':
      return Pattern(0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001, 0b10001);
    case 'O':
      return Pattern(0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110);
    case 'P':
      return Pattern(0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000);
    case 'Q':
      return Pattern(0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101);
    case 'R':
      return Pattern(0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001);
    case 'S':
      return Pattern(0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110);
    case 'T':
      return Pattern(0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100);
    case 'U':
      return Pattern(0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110);
    case 'V':
      return Pattern(0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100);
    case 'W':
      return Pattern(0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010);
    case 'X':
      return Pattern(0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001);
    case 'Y':
      return Pattern(0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100, 0b00100);
    case 'Z':
      return Pattern(0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111);
    case '0':
      return Pattern(0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110);
    case '1':
      return Pattern(0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110);
    case '2':
      return Pattern(0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111);
    case '3':
      return Pattern(0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110);
    case '4':
      return Pattern(0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010);
    case '5':
      return Pattern(0b11111, 0b10000, 0b10000, 0b11110, 0b00001, 0b00001, 0b11110);
    case '6':
      return Pattern(0b01110, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110);
    case '7':
      return Pattern(0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000);
    case '8':
      return Pattern(0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110);
    case '9':
      return Pattern(0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b01110);
    case ':':
      return Pattern(0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000);
    case '/':
      return Pattern(0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000);
    case '-':
      return Pattern(0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000);
    case '<':
      return Pattern(0b00010, 0b00100, 0b01000, 0b10000, 0b01000, 0b00100, 0b00010);
    case '>':
      return Pattern(0b01000, 0b00100, 0b00010, 0b00001, 0b00010, 0b00100, 0b01000);
    default:
      return Pattern(0b00000, 0b11111, 0b00001, 0b00110, 0b00100, 0b00000, 0b00100);
  }
}

std::array<std::uint8_t, 16> BuildGlyph(std::uint64_t pattern) noexcept {
  std::array<std::uint8_t, 16> glyph{};
  for (std::uint8_t row = 0; row < 7; row++) {
    const auto row_pattern = static_cast<std::uint8_t>((pattern >> ((6 - row) * 5)) & 0x1F);
    const std::uint8_t output_row = static_cast<std::uint8_t>(row + 1U);
    glyph[output_row * 2] = static_cast<std::uint8_t>(row_pattern << 2);
    glyph[output_row * 2 + 1] = static_cast<std::uint8_t>(row_pattern << 2);
  }
  return glyph;
}

}  // namespace

std::span<const std::uint8_t, 16> Font8x16Glyph(char character) noexcept {
  static std::array<std::uint8_t, 16> glyph{};
  glyph = BuildGlyph(GlyphPattern(character));
  return glyph;
}

}  // namespace midismith::main_board::app::ui
