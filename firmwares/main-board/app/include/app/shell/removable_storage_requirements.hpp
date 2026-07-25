#pragma once

namespace midismith::main_board::app::shell {

class RemovableStorageRequirements {
 public:
  virtual ~RemovableStorageRequirements() = default;

  [[nodiscard]] virtual bool IsCardPresent() const noexcept = 0;

  [[nodiscard]] virtual bool Mount() noexcept = 0;

  virtual void Unmount() noexcept = 0;
};

}  // namespace midismith::main_board::app::shell
