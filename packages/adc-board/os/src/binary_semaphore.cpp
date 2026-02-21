#include "os/binary_semaphore.hpp"

#include "cmsis_os2.h"

namespace os {

static osSemaphoreId_t s(void* sem) {
  return reinterpret_cast<osSemaphoreId_t>(sem);
}

BinarySemaphore::BinarySemaphore() noexcept : _sem(nullptr) {
  _sem = reinterpret_cast<void*>(osSemaphoreNew(1U, 0U, nullptr));
}

BinarySemaphore::~BinarySemaphore() noexcept {
  if (_sem) {
    osSemaphoreDelete(s(_sem));
    _sem = nullptr;
  }
}

bool BinarySemaphore::acquire(std::uint32_t timeout_ms) noexcept {
  if (!_sem) {
    return false;
  }
  return osSemaphoreAcquire(s(_sem), timeout_ms) == osOK;
}

bool BinarySemaphore::release() noexcept {
  if (!_sem) {
    return false;
  }
  return osSemaphoreRelease(s(_sem)) == osOK;
}

}  // namespace os
