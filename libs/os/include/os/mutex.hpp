#pragma once

#include "os-types/mutex_requirements.hpp"

namespace midismith::os {

class Mutex final : public MutexRequirements {
 public:
  Mutex() noexcept;
  ~Mutex() noexcept;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  void Lock() noexcept override;
  void Unlock() noexcept override;

 private:
  void* handle_;
};

}  // namespace midismith::os
