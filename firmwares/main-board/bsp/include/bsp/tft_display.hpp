#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "bsp/gpio_requirements.hpp"

namespace midismith::main_board::bsp {

class TftDisplay {
 public:
  using DelayMs = void (*)(void* context, std::uint32_t milliseconds) noexcept;

  TftDisplay(void* spi_handle, midismith::bsp::GpioRequirements& chip_select,
             midismith::bsp::GpioRequirements& data_command,
             midismith::bsp::GpioRequirements& backlight, DelayMs delay_ms,
             void* delay_context) noexcept;

  void Init() noexcept;
  void SetBacklight(bool enabled) noexcept;
  void FillRect(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height,
                std::uint16_t color565) noexcept;
  void BlitBitmap(std::uint16_t x, std::uint16_t y, std::uint16_t width, std::uint16_t height,
                  const std::uint8_t* mono_bitmap, std::uint16_t foreground565,
                  std::uint16_t background565) noexcept;
  void BlitRows(std::uint16_t y, std::uint16_t row_count, const std::uint8_t* pixel_bytes) noexcept;

  std::uint16_t width() const noexcept;
  std::uint16_t height() const noexcept;

 private:
  static constexpr std::size_t kBurstBufferBytes = 512;

  void WriteCommand(std::uint8_t command) noexcept;
  void WriteData(std::uint8_t value) noexcept;
  void WriteData(const std::uint8_t* data, std::uint16_t size) noexcept;
  void SetAddressWindow(std::uint16_t x, std::uint16_t y, std::uint16_t width,
                        std::uint16_t height) noexcept;
  void WriteColor(std::uint16_t color565, std::uint32_t count) noexcept;

  std::array<std::uint8_t, kBurstBufferBytes> burst_buffer_{};
  void* spi_handle_;
  midismith::bsp::GpioRequirements& chip_select_;
  midismith::bsp::GpioRequirements& data_command_;
  midismith::bsp::GpioRequirements& backlight_;
  DelayMs delay_ms_;
  void* delay_context_;
};

}  // namespace midismith::main_board::bsp
