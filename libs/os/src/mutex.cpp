#include "os/mutex.hpp"

#include "cmsis_os2.h"

namespace midismith::os {

namespace {

osMutexId_t m(void* handle) {
  return reinterpret_cast<osMutexId_t>(handle);
}

}  // namespace

Mutex::Mutex() noexcept : handle_(nullptr) {
  handle_ = reinterpret_cast<void*>(osMutexNew(nullptr));
}

Mutex::~Mutex() noexcept {
  if (handle_) {
    osMutexDelete(m(handle_));
    handle_ = nullptr;
  }
}

void Mutex::Lock() noexcept {
  if (handle_) {
    osMutexAcquire(m(handle_), osWaitForever);
  }
}

void Mutex::Unlock() noexcept {
  if (handle_) {
    osMutexRelease(m(handle_));
  }
}

}  // namespace midismith::os
