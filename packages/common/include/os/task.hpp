#pragma once

#include <cstddef>
#include <cstdint>

namespace os {

using TaskFn = void (*)(void* ctx) noexcept;

class Task {
 public:
  static bool create(const char* name, TaskFn fn, void* arg, std::size_t stack_bytes,
                     std::uint32_t priority) noexcept;
};

}  // namespace os

namespace midismith::common::os {
using ::os::Task;
using ::os::TaskFn;
}  // namespace midismith::common::os
