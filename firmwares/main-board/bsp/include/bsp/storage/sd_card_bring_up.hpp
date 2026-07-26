#pragma once

#include "bsp-types/storage/sd_card_bring_up_outcome.hpp"

namespace midismith::main_board::bsp::storage {

void BeginSdCardBringUpAttempt() noexcept;

[[nodiscard]] midismith::bsp::storage::SdCardBringUpOutcome LastSdCardBringUpOutcome() noexcept;

}  // namespace midismith::main_board::bsp::storage
