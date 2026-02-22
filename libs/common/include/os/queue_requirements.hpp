#pragma once

#include <cstdint>

namespace midismith::common::os {

/**
 * @brief Constants for queue operation timeouts.
 */
constexpr uint32_t kWaitForever = 0xFFFFFFFFU;
constexpr uint32_t kNoWait = 0;

/**
 * @brief Interface for a thread-safe message queue.
 */
template <typename T>
class QueueRequirements {
 public:
  virtual ~QueueRequirements() = default;

  /**
   * @brief Sends an item to the queue.
   * @param item The item to copy into the queue.
   * @param timeout_ms Max time to wait if queue is full.
   * @return true if item was sent, false on timeout or error.
   */
  virtual bool Send(const T& item, uint32_t timeout_ms) noexcept = 0;

  /**
   * @brief Receives an item from the queue.
   * @param item Reference to store the received item.
   * @param timeout_ms Max time to wait if queue is empty.
   * @return true if item was received, false on timeout or error.
   */
  virtual bool Receive(T& item, uint32_t timeout_ms) noexcept = 0;
};

}  // namespace midismith::common::os
