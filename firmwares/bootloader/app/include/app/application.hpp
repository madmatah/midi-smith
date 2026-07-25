#pragma once

namespace midismith::bootloader::app {

class Application {
 public:
  [[noreturn]] static void Run() noexcept;
};

}  // namespace midismith::bootloader::app
