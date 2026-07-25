#pragma once

#include <cstdint>

namespace midismith::bootloader::bsp {

class ApplicationLauncher {
 public:
  [[noreturn]] static void LaunchAt(std::uint32_t application_address) noexcept;
};

}  // namespace midismith::bootloader::bsp
