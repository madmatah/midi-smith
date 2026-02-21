#include "os/task.hpp"

#include "cmsis_os2.h"

namespace os {

static osPriority_t to_priority(std::uint32_t p) {
  if (p >= static_cast<std::uint32_t>(osPriorityRealtime)) {
    return osPriorityRealtime;
  }
  return static_cast<osPriority_t>(p);
}

bool Task::create(const char* name, TaskFn fn, void* arg, std::size_t stack_bytes,
                  std::uint32_t priority) noexcept {
  if (fn == nullptr) {
    return false;
  }

  osThreadAttr_t attr{};
  attr.name = name;
  attr.stack_size = stack_bytes;
  attr.priority = to_priority(priority);

  return osThreadNew(reinterpret_cast<osThreadFunc_t>(fn), arg, &attr) != nullptr;
}

}  // namespace os
