#include "os/timer.hpp"

#include "cmsis_os2.h"

namespace midismith::os {

namespace {

osTimerId_t timer_id(void* id) {
  return reinterpret_cast<osTimerId_t>(id);
}

}  // namespace

Timer::Timer(Callback callback, void* ctx) noexcept : id_(nullptr) {
  id_ = reinterpret_cast<void*>(
      osTimerNew(reinterpret_cast<osTimerFunc_t>(callback), osTimerPeriodic, ctx, nullptr));
}

Timer::~Timer() noexcept {
  if (id_) {
    osTimerDelete(timer_id(id_));
    id_ = nullptr;
  }
}

bool Timer::Start(std::uint32_t period_ms) noexcept {
  if (!id_) {
    return false;
  }
  const std::uint32_t ticks = period_ms * osKernelGetTickFreq() / 1000u;
  return osTimerStart(timer_id(id_), ticks) == osOK;
}

bool Timer::Stop() noexcept {
  if (!id_) {
    return false;
  }
  return osTimerStop(timer_id(id_)) == osOK;
}

}  // namespace midismith::os
