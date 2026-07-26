#pragma once

#include "bsp-types/storage/sd_card_bring_up_outcome.hpp"
#include "bsp-types/storage/volume_mount_result.hpp"

namespace midismith::main_board::app::shell {

class RemovableStorageRequirements {
 public:
  virtual ~RemovableStorageRequirements() = default;

  [[nodiscard]] virtual bool Mount() noexcept = 0;

  virtual void Unmount() noexcept = 0;

  [[nodiscard]] virtual midismith::bsp::storage::SdCardBringUpOutcome last_bring_up_outcome()
      const noexcept = 0;

  [[nodiscard]] virtual midismith::bsp::storage::VolumeMountResult last_mount_result()
      const noexcept = 0;
};

}  // namespace midismith::main_board::app::shell
