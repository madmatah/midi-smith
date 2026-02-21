#include "app/app_entry.h"

#include "app/application.hpp"

extern "C" void AppEntry_Init(void) {
  app::Application::init();
}

extern "C" void AppEntry_Loop(void) {}

extern "C" void AppEntry_CreateTasks(void) {
  app::Application::create_tasks();
}
