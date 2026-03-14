#pragma once

namespace midismith::os {

class MutexRequirements {
 public:
  virtual ~MutexRequirements() = default;
  virtual void Lock() noexcept = 0;
  virtual void Unlock() noexcept = 0;
};

}  // namespace midismith::os
