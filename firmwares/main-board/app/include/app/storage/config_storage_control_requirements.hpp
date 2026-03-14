#pragma once

namespace midismith::main_board::app::storage {

class ConfigStorageControlRequirements {
 public:
  virtual ~ConfigStorageControlRequirements() = default;
  virtual void RequestPersist() noexcept = 0;
};

}  // namespace midismith::main_board::app::storage
