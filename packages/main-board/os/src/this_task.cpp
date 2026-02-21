#include "os/this_task.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace os {

std::uint32_t ThisTask::stack_high_water_mark_bytes() noexcept {
  return static_cast<std::uint32_t>(uxTaskGetStackHighWaterMark(nullptr) * sizeof(StackType_t));
}

}  // namespace os
