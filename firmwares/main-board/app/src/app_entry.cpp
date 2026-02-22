#include "app/app_entry.h"

#include "app/application.hpp"

extern "C" void AppEntry_Init(void) {
  midismith::main_board::app::Application::init();
}

extern "C" void AppEntry_Loop(void) {}

extern "C" void AppEntry_CreateTasks(void) {
  midismith::main_board::app::Application::create_tasks();
}
