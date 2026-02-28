#pragma once

#include <cstdint>

namespace midismith::main_board::app::config {

// CAN
constexpr uint32_t CAN_RECEIVE_QUEUE_CAPACITY = 16;
constexpr uint32_t CAN_TASK_STACK_BYTES = 1024;
constexpr uint32_t CAN_TASK_PRIORITY = 2;

}  // namespace midismith::main_board::app::config
