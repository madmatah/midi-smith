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

// Calibration
// CalibrationDataServerTask loads calibration storage metadata and still needs headroom for
// protocol packing and queue dispatch.
constexpr std::uint32_t CALIBRATION_SERVER_TASK_STACK_BYTES = 1536;
constexpr std::uint32_t CALIBRATION_SERVER_TASK_PRIORITY = 1;
constexpr std::uint32_t kCalibrationServerEventQueueCapacity = 16;
constexpr std::uint32_t kCalibrationServerAckTimeoutMs = 100;
constexpr std::uint32_t kCalibrationServerMaxRetries = 3;
constexpr std::uint32_t kCalibrationReceiveTimeoutMs = 500;
constexpr float kMaxValidStrikeCurrentMa = 1.138f;
constexpr std::uint32_t kCalibrationRestDurationMs = 2000;

// ADC board lifecycle
constexpr bool kAutoStartPowerSequenceOnBoot = false;
constexpr std::uint32_t kPowerOnTimeoutMs = 5000;
constexpr std::size_t kMaxPeerCount = 8;

}  // namespace midismith::main_board::app::config
