#pragma once

#include <cstdint>

namespace midismith::main_board::app::config {

// CAN
constexpr uint32_t CAN_RECEIVE_QUEUE_CAPACITY = 16;
constexpr uint32_t CAN_TASK_STACK_BYTES = 1024;
constexpr uint32_t CAN_TASK_PRIORITY = 2;

// Config storage
// StorageManager::Save stack-allocates 2 * sizeof(MainBoardConfig) = 1152 bytes
constexpr uint32_t CONFIG_STORAGE_TASK_STACK_BYTES = 2048;
constexpr uint32_t CONFIG_STORAGE_TASK_PRIORITY = 1;

// Supervisor
constexpr uint32_t SUPERVISOR_TASK_STACK_BYTES = 512;
constexpr uint32_t SUPERVISOR_TASK_PRIORITY = 1;
constexpr uint32_t kHeartbeatPeriodMs = 500;
constexpr uint32_t kHeartbeatTimeoutMs = 1500;
constexpr uint32_t kTimeoutCheckPeriodMs = 100;

// ADC board lifecycle
constexpr bool kAutoStartPowerSequenceOnBoot = false;
constexpr std::uint32_t kPowerOnTimeoutMs = 5000;
constexpr std::size_t kMaxPeerCount = 8;

}  // namespace midismith::main_board::app::config
