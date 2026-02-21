#pragma once

#include <cstdint>

#include "cmsis_os2.h"
#include "os/queue_requirements.hpp"

namespace os {

template <typename T, uint32_t Capacity>
class Queue : public QueueRequirements<T> {
 public:
  Queue() noexcept {
    osMessageQueueAttr_t attributes{};
    attributes.cb_mem = &control_block_memory_;
    attributes.cb_size = sizeof(control_block_memory_);
    attributes.mq_mem = &queue_memory_;
    attributes.mq_size = sizeof(queue_memory_);

    id_ = osMessageQueueNew(Capacity, sizeof(T), &attributes);
  }

  ~Queue() override {
    if (id_ != nullptr) {
      osMessageQueueDelete(id_);
    }
  }

  Queue(const Queue&) = delete;
  Queue& operator=(const Queue&) = delete;

  bool Send(const T& item, uint32_t timeout_ms) noexcept override {
    if (id_ == nullptr) {
      return false;
    }
    return osMessageQueuePut(id_, &item, 0, timeout_ms) == osOK;
  }

  bool Receive(T& item, uint32_t timeout_ms) noexcept override {
    if (id_ == nullptr) {
      return false;
    }
    return osMessageQueueGet(id_, &item, nullptr, timeout_ms) == osOK;
  }

 private:
  osMessageQueueId_t id_{nullptr};

  static constexpr uint32_t kControlBlockSize = 128;
  // Raw buffer used to avoid exposing FreeRTOS headers in the public interface.
  alignas(4) uint8_t control_block_memory_[kControlBlockSize];
  alignas(4) uint8_t queue_memory_[Capacity * sizeof(T)];
};

}  // namespace os
