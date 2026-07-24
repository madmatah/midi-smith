#include "bsp/tft_display.hpp"

#include <array>
#include <cstddef>

#include "stm32h7xx_hal.h"

namespace midismith::main_board::bsp {

namespace {

constexpr std::uint16_t kDisplayWidth = 160;
constexpr std::uint16_t kDisplayHeight = 80;
// The 80x160 panel sits centered in the ST7735S 132x162 RAM; landscape swaps the offsets.
constexpr std::uint16_t kColumnAddressOffset = 1;
constexpr std::uint16_t kRowAddressOffset = 26;
constexpr std::uint32_t kSpiTimeoutMs = 100;
constexpr std::uint8_t kSoftwareReset = 0x01;
constexpr std::uint8_t kSleepOut = 0x11;
// This IPS panel is normally-white: colors are displayed inverted unless INVON is set.
constexpr std::uint8_t kInversionOn = 0x21;
constexpr std::uint8_t kColorMode = 0x3A;
constexpr std::uint8_t kMemoryAccessControl = 0x36;
constexpr std::uint8_t kDisplayOn = 0x29;
constexpr std::uint8_t kColumnAddressSet = 0x2A;
constexpr std::uint8_t kRowAddressSet = 0x2B;
constexpr std::uint8_t kMemoryWrite = 0x2C;
constexpr std::uint8_t kRgb565ColorMode = 0x05;
constexpr std::uint8_t kMemoryAccessLandscapeBgr = 0xA8;

SPI_HandleTypeDef* SpiHandle(void* handle) noexcept {
  return reinterpret_cast<SPI_HandleTypeDef*>(handle);
}

std::uint8_t HighByte(std::uint16_t value) noexcept {
  return static_cast<std::uint8_t>(value >> 8);
}

std::uint8_t LowByte(std::uint16_t value) noexcept {
  return static_cast<std::uint8_t>(value & 0xFF);
}

}  // namespace

TftDisplay::TftDisplay(void* spi_handle, midismith::bsp::GpioRequirements& chip_select,
                       midismith::bsp::GpioRequirements& data_command,
                       midismith::bsp::GpioRequirements& backlight, DelayMs delay_ms,
                       void* delay_context) noexcept
    : spi_handle_(spi_handle),
      chip_select_(chip_select),
      data_command_(data_command),
      backlight_(backlight),
      delay_ms_(delay_ms),
      delay_context_(delay_context) {}

void TftDisplay::Init() noexcept {
  chip_select_.set();
  data_command_.reset();
  SetBacklight(false);
  delay_ms_(delay_context_, 20);
  WriteCommand(kSoftwareReset);
  delay_ms_(delay_context_, 150);
  WriteCommand(kSleepOut);
  delay_ms_(delay_context_, 120);
  WriteCommand(kInversionOn);
  WriteCommand(kColorMode);
  WriteData(kRgb565ColorMode);
  WriteCommand(kMemoryAccessControl);
  WriteData(kMemoryAccessLandscapeBgr);
  WriteCommand(kDisplayOn);
  delay_ms_(delay_context_, 20);
  FillRect(0, 0, width(), height(), 0x0000);
  SetBacklight(true);
}

void TftDisplay::SetBacklight(bool enabled) noexcept {
  if (enabled) {
    backlight_.reset();
  } else {
    backlight_.set();
  }
}

void TftDisplay::FillRect(std::uint16_t x, std::uint16_t y, std::uint16_t width,
                          std::uint16_t height, std::uint16_t color565) noexcept {
  if (x >= kDisplayWidth || y >= kDisplayHeight || width == 0 || height == 0) {
    return;
  }
  const std::uint16_t clipped_width =
      x + width > kDisplayWidth ? static_cast<std::uint16_t>(kDisplayWidth - x) : width;
  const std::uint16_t clipped_height =
      y + height > kDisplayHeight ? static_cast<std::uint16_t>(kDisplayHeight - y) : height;
  SetAddressWindow(x, y, clipped_width, clipped_height);
  WriteColor(color565, static_cast<std::uint32_t>(clipped_width) * clipped_height);
}

void TftDisplay::BlitBitmap(std::uint16_t x, std::uint16_t y, std::uint16_t width,
                            std::uint16_t height, const std::uint8_t* mono_bitmap,
                            std::uint16_t foreground565, std::uint16_t background565) noexcept {
  if (mono_bitmap == nullptr || width == 0 || height == 0) {
    return;
  }
  SetAddressWindow(x, y, width, height);
  const std::uint16_t bytes_per_row = static_cast<std::uint16_t>((width + 7) / 8);
  std::size_t buffered_bytes = 0;
  for (std::uint16_t row = 0; row < height; row++) {
    for (std::uint16_t column = 0; column < width; column++) {
      const std::uint8_t byte = mono_bitmap[row * bytes_per_row + column / 8];
      const bool pixel_enabled = (byte & (0x80 >> (column % 8))) != 0;
      const std::uint16_t color = pixel_enabled ? foreground565 : background565;
      burst_buffer_[buffered_bytes] = HighByte(color);
      burst_buffer_[buffered_bytes + 1] = LowByte(color);
      buffered_bytes += 2;
      if (buffered_bytes == burst_buffer_.size()) {
        WriteData(burst_buffer_.data(), static_cast<std::uint16_t>(buffered_bytes));
        buffered_bytes = 0;
      }
    }
  }
  if (buffered_bytes > 0) {
    WriteData(burst_buffer_.data(), static_cast<std::uint16_t>(buffered_bytes));
  }
}

void TftDisplay::BlitRows(std::uint16_t y, std::uint16_t row_count,
                          const std::uint8_t* pixel_bytes) noexcept {
  if (pixel_bytes == nullptr || row_count == 0 || y >= kDisplayHeight) {
    return;
  }
  const std::uint16_t clipped_row_count =
      y + row_count > kDisplayHeight ? static_cast<std::uint16_t>(kDisplayHeight - y) : row_count;
  SetAddressWindow(0, y, kDisplayWidth, clipped_row_count);
  const std::uint32_t total_bytes =
      static_cast<std::uint32_t>(clipped_row_count) * kDisplayWidth * 2;
  WriteData(pixel_bytes, static_cast<std::uint16_t>(total_bytes));
}

std::uint16_t TftDisplay::width() const noexcept {
  return kDisplayWidth;
}

std::uint16_t TftDisplay::height() const noexcept {
  return kDisplayHeight;
}

void TftDisplay::WriteCommand(std::uint8_t command) noexcept {
  chip_select_.reset();
  data_command_.reset();
  HAL_SPI_Transmit(SpiHandle(spi_handle_), &command, 1, kSpiTimeoutMs);
  chip_select_.set();
}

void TftDisplay::WriteData(std::uint8_t value) noexcept {
  WriteData(&value, 1);
}

void TftDisplay::WriteData(const std::uint8_t* data, std::uint16_t size) noexcept {
  chip_select_.reset();
  data_command_.set();
  HAL_SPI_Transmit(SpiHandle(spi_handle_), const_cast<std::uint8_t*>(data), size, kSpiTimeoutMs);
  chip_select_.set();
}

void TftDisplay::SetAddressWindow(std::uint16_t x, std::uint16_t y, std::uint16_t width,
                                  std::uint16_t height) noexcept {
  const std::uint16_t x_start = static_cast<std::uint16_t>(x + kColumnAddressOffset);
  const std::uint16_t y_start = static_cast<std::uint16_t>(y + kRowAddressOffset);
  const std::uint16_t x_end = static_cast<std::uint16_t>(x_start + width - 1);
  const std::uint16_t y_end = static_cast<std::uint16_t>(y_start + height - 1);
  const std::array<std::uint8_t, 4> column_data{HighByte(x_start), LowByte(x_start),
                                                HighByte(x_end), LowByte(x_end)};
  const std::array<std::uint8_t, 4> row_data{HighByte(y_start), LowByte(y_start), HighByte(y_end),
                                             LowByte(y_end)};
  WriteCommand(kColumnAddressSet);
  WriteData(column_data.data(), column_data.size());
  WriteCommand(kRowAddressSet);
  WriteData(row_data.data(), row_data.size());
  WriteCommand(kMemoryWrite);
}

void TftDisplay::WriteColor(std::uint16_t color565, std::uint32_t count) noexcept {
  const std::size_t buffer_pixel_capacity = burst_buffer_.size() / 2;
  const std::size_t prefilled_pixels =
      count < buffer_pixel_capacity ? static_cast<std::size_t>(count) : buffer_pixel_capacity;
  for (std::size_t pixel = 0; pixel < prefilled_pixels; pixel++) {
    burst_buffer_[pixel * 2] = HighByte(color565);
    burst_buffer_[pixel * 2 + 1] = LowByte(color565);
  }
  std::uint32_t remaining_pixels = count;
  while (remaining_pixels > 0) {
    const std::size_t chunk_pixels = remaining_pixels < buffer_pixel_capacity
                                         ? static_cast<std::size_t>(remaining_pixels)
                                         : buffer_pixel_capacity;
    WriteData(burst_buffer_.data(), static_cast<std::uint16_t>(chunk_pixels * 2));
    remaining_pixels -= static_cast<std::uint32_t>(chunk_pixels);
  }
}

}  // namespace midismith::main_board::bsp
