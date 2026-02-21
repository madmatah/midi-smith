#pragma once

#include "app/analog/acquisition_command.hpp"
#include "app/analog/acquisition_control_requirements.hpp"
#include "os/queue.hpp"

namespace app::analog {

class QueueAcquisitionControl final : public AcquisitionControlRequirements {
 public:
  QueueAcquisitionControl(os::Queue<AcquisitionCommand, 4>& queue,
                          volatile AcquisitionState& state) noexcept
      : queue_(queue), state_(state) {}

  bool RequestEnable() noexcept override {
    if (state_ == AcquisitionState::kEnabled) {
      return true;
    }
    return queue_.Send(AcquisitionCommand::kEnable, os::kNoWait);
  }

  bool RequestDisable() noexcept override {
    if (state_ == AcquisitionState::kDisabled) {
      return true;
    }
    return queue_.Send(AcquisitionCommand::kDisable, os::kNoWait);
  }

  AcquisitionState GetState() const noexcept override {
    return state_;
  }

 private:
  os::Queue<AcquisitionCommand, 4>& queue_;
  volatile AcquisitionState& state_;
};

}  // namespace app::analog
