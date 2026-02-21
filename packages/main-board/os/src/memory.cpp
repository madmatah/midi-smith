#include "os/memory.hpp"

#include "FreeRTOS.h"
#include "portable.h"

namespace os {

std::uint32_t Memory::free_heap_bytes() noexcept {
  return static_cast<std::uint32_t>(xPortGetFreeHeapSize());
}

std::uint32_t Memory::min_ever_free_heap_bytes() noexcept {
  return static_cast<std::uint32_t>(xPortGetMinimumEverFreeHeapSize());
}

}  // namespace os
