#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace midismith::bootloader::bsp {

class InternalFlash {
 public:
  static constexpr std::size_t kFlashWordSizeBytes = 32;

  [[nodiscard]] static std::span<const std::uint8_t> ReadRegion(std::uint32_t address,
                                                                std::size_t length_bytes) noexcept;

  [[nodiscard]] static bool EraseRegion(std::uint32_t address, std::size_t length_bytes) noexcept;

  [[nodiscard]] static bool ProgramRegion(std::uint32_t address,
                                          std::span<const std::uint8_t> data) noexcept;
};

}  // namespace midismith::bootloader::bsp
