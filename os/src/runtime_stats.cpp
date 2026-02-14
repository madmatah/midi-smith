#include "os/runtime_stats.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "task.h"

namespace os {
namespace {

constexpr UBaseType_t kSnapshotCapacityMargin = 4u;
constexpr std::uint32_t kCpuScalePermille = 1000u;
constexpr std::uint32_t kSnapshotRetryCount = 3u;

struct RuntimeSnapshot {
  TaskStatus_t* tasks{nullptr};
  UBaseType_t task_count{0u};
  UBaseType_t task_capacity{0u};
  std::uint32_t total_runtime{0u};
  bool truncated{false};
};

std::uint32_t Delta(std::uint32_t newer_value, std::uint32_t older_value) noexcept {
  return newer_value - older_value;
}

void ReleaseSnapshot(RuntimeSnapshot& snapshot) noexcept {
  if (snapshot.tasks != nullptr) {
    vPortFree(snapshot.tasks);
  }
  snapshot.tasks = nullptr;
  snapshot.task_count = 0u;
  snapshot.task_capacity = 0u;
  snapshot.total_runtime = 0u;
  snapshot.truncated = false;
}

bool AllocateSnapshotTasks(RuntimeSnapshot& snapshot, UBaseType_t task_capacity) noexcept {
  if (task_capacity == 0u) {
    return false;
  }

  snapshot.tasks = static_cast<TaskStatus_t*>(pvPortMalloc(sizeof(TaskStatus_t) * task_capacity));
  if (snapshot.tasks == nullptr) {
    return false;
  }

  snapshot.task_capacity = task_capacity;
  return true;
}

bool CaptureSnapshot(RuntimeSnapshot& snapshot) noexcept {
  ReleaseSnapshot(snapshot);

  UBaseType_t estimated_task_count = uxTaskGetNumberOfTasks();
  for (std::uint32_t attempt = 0u; attempt < kSnapshotRetryCount; ++attempt) {
    const UBaseType_t task_capacity = estimated_task_count + kSnapshotCapacityMargin;
    if (!AllocateSnapshotTasks(snapshot, task_capacity)) {
      ReleaseSnapshot(snapshot);
      return false;
    }

    snapshot.total_runtime = 0u;
    snapshot.task_count =
        uxTaskGetSystemState(snapshot.tasks, snapshot.task_capacity, &snapshot.total_runtime);

    const UBaseType_t observed_task_count = uxTaskGetNumberOfTasks();
    snapshot.truncated = (snapshot.task_count >= snapshot.task_capacity) ||
                         (observed_task_count > snapshot.task_capacity);

    if (!snapshot.truncated) {
      return true;
    }

    estimated_task_count = observed_task_count;
    ReleaseSnapshot(snapshot);
  }

  return false;
}

const TaskStatus_t* FindTaskByHandle(const RuntimeSnapshot& snapshot,
                                     TaskHandle_t handle) noexcept {
  for (UBaseType_t i = 0; i < snapshot.task_count; ++i) {
    if (snapshot.tasks[i].xHandle == handle) {
      return &snapshot.tasks[i];
    }
  }
  return nullptr;
}

bool IsIdleTaskName(const char* task_name) noexcept {
  if (task_name == nullptr) {
    return false;
  }
  return std::strncmp(task_name, "IDLE", 4u) == 0;
}

char MapTaskStateCode(eTaskState task_state) noexcept {
  if (task_state == eRunning) {
    return 'R';
  }
  if (task_state == eReady) {
    return 'Y';
  }
  if (task_state == eBlocked) {
    return 'B';
  }
  if (task_state == eSuspended) {
    return 'S';
  }
  if (task_state == eDeleted) {
    return 'D';
  }
  return 'I';
}

void CopyTaskName(const char* task_name, char* output) noexcept {
  if (output == nullptr) {
    return;
  }
  output[0] = '\0';
  if (task_name == nullptr) {
    return;
  }

  const std::size_t max_characters = kRuntimeTaskNameCapacity - 1u;
  std::strncpy(output, task_name, max_characters);
  output[max_characters] = '\0';
}

void SwapRows(RuntimeTaskSnapshotRow& lhs, RuntimeTaskSnapshotRow& rhs) noexcept {
  RuntimeTaskSnapshotRow tmp = lhs;
  lhs = rhs;
  rhs = tmp;
}

void SortRowsByCpu(RuntimeTaskSnapshotRow* task_rows, std::size_t task_row_count) noexcept {
  for (std::size_t i = 0; i < task_row_count; ++i) {
    std::size_t best_index = i;
    for (std::size_t j = i + 1u; j < task_row_count; ++j) {
      if (task_rows[j].cpu_load_permille > task_rows[best_index].cpu_load_permille) {
        best_index = j;
      }
    }
    if (best_index != i) {
      SwapRows(task_rows[i], task_rows[best_index]);
    }
  }
}

}  // namespace

bool RuntimeStats::CaptureStatusSnapshot(std::uint32_t window_ms,
                                         RuntimeStatusSnapshot& status_snapshot) noexcept {
  RuntimeSnapshot start{};
  RuntimeSnapshot end{};

  if (!CaptureSnapshot(start)) {
    return false;
  }

  osDelay(window_ms);

  if (!CaptureSnapshot(end)) {
    ReleaseSnapshot(start);
    return false;
  }

  const std::uint32_t total_runtime_delta = Delta(end.total_runtime, start.total_runtime);
  if (total_runtime_delta == 0u) {
    ReleaseSnapshot(start);
    ReleaseSnapshot(end);
    return false;
  }

  std::uint64_t idle_runtime_delta = 0u;
  for (UBaseType_t i = 0; i < end.task_count; ++i) {
    const TaskStatus_t& current = end.tasks[i];
    if (!IsIdleTaskName(current.pcTaskName)) {
      continue;
    }

    const TaskStatus_t* previous = FindTaskByHandle(start, current.xHandle);
    if (previous == nullptr) {
      continue;
    }
    idle_runtime_delta += Delta(current.ulRunTimeCounter, previous->ulRunTimeCounter);
  }

  std::uint32_t cpu_load_permille = kCpuScalePermille;
  const std::uint64_t idle_load_permille =
      (idle_runtime_delta * kCpuScalePermille) / total_runtime_delta;
  if (idle_load_permille >= kCpuScalePermille) {
    cpu_load_permille = 0u;
  } else {
    cpu_load_permille = kCpuScalePermille - static_cast<std::uint32_t>(idle_load_permille);
  }

  status_snapshot.cpu_load_permille = cpu_load_permille;
  status_snapshot.window_ms = window_ms;
  status_snapshot.task_count = static_cast<std::uint32_t>(end.task_count);
  status_snapshot.heap_free_bytes = xPortGetFreeHeapSize();
  status_snapshot.heap_min_bytes = xPortGetMinimumEverFreeHeapSize();
  status_snapshot.uptime_ms =
      (static_cast<std::uint64_t>(xTaskGetTickCount()) * 1000u) / configTICK_RATE_HZ;
  status_snapshot.truncated = start.truncated || end.truncated;

  ReleaseSnapshot(start);
  ReleaseSnapshot(end);
  return true;
}

bool RuntimeStats::CaptureTaskSnapshotRows(std::uint32_t window_ms,
                                           RuntimeTaskSnapshotRow* task_rows,
                                           std::size_t max_task_rows, std::size_t& task_row_count,
                                           bool& snapshot_truncated) noexcept {
  task_row_count = 0u;
  snapshot_truncated = false;
  if (task_rows == nullptr || max_task_rows == 0u) {
    return false;
  }

  RuntimeSnapshot start{};
  RuntimeSnapshot end{};
  if (!CaptureSnapshot(start)) {
    return false;
  }

  osDelay(window_ms);

  if (!CaptureSnapshot(end)) {
    ReleaseSnapshot(start);
    return false;
  }

  const std::uint32_t total_runtime_delta = Delta(end.total_runtime, start.total_runtime);
  if (total_runtime_delta == 0u) {
    ReleaseSnapshot(start);
    ReleaseSnapshot(end);
    return false;
  }

  const std::size_t max_rows = (static_cast<std::size_t>(end.task_count) < max_task_rows)
                                   ? static_cast<std::size_t>(end.task_count)
                                   : max_task_rows;

  for (std::size_t i = 0; i < max_rows; ++i) {
    const TaskStatus_t& current = end.tasks[i];
    const TaskStatus_t* previous = FindTaskByHandle(start, current.xHandle);

    std::uint32_t runtime_delta = 0u;
    if (previous != nullptr) {
      runtime_delta = Delta(current.ulRunTimeCounter, previous->ulRunTimeCounter);
    }

    std::uint32_t cpu_load_permille = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(runtime_delta) * kCpuScalePermille) / total_runtime_delta);
    if (cpu_load_permille > kCpuScalePermille) {
      cpu_load_permille = kCpuScalePermille;
    }

    RuntimeTaskSnapshotRow& row = task_rows[task_row_count];
    CopyTaskName(current.pcTaskName, row.task_name);
    row.cpu_load_permille = cpu_load_permille;
    row.stack_free_bytes =
        static_cast<std::uint32_t>(current.usStackHighWaterMark) * sizeof(StackType_t);
    row.priority = current.uxCurrentPriority;
    row.state_code = MapTaskStateCode(current.eCurrentState);
    row.runtime_delta = runtime_delta;
    task_row_count++;
  }

  SortRowsByCpu(task_rows, task_row_count);
  snapshot_truncated = start.truncated || end.truncated ||
                       (static_cast<std::size_t>(end.task_count) > max_task_rows);

  ReleaseSnapshot(start);
  ReleaseSnapshot(end);
  return true;
}

}  // namespace os
