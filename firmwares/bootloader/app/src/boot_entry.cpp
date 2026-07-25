#include "app/boot_entry.h"

#include "app/application.hpp"

extern "C" void BootEntry_Run(void) {
  midismith::bootloader::app::Application::Run();
}
