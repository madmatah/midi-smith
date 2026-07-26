#include "bsp/storage/staging_slot_flash.hpp"

#include "bsp-flash/internal_flash.hpp"
#include "flash-layout/flash_layout.hpp"

namespace midismith::main_board::bsp::storage {

namespace {

using midismith::bsp_flash::InternalFlash;

}  // namespace

std::size_t StagingSlotFlash::CapacityBytes() const noexcept {
  return midismith::flash_layout::kStagingSizeBytes;
}

bool StagingSlotFlash::Erase() noexcept {
  return InternalFlash::EraseRegion(midismith::flash_layout::kStagingAddress,
                                    midismith::flash_layout::kStagingSizeBytes);
}

bool StagingSlotFlash::ProgramFlashWord(std::size_t offset_bytes,
                                        std::span<const std::uint8_t> word) noexcept {
  if (offset_bytes + word.size() > midismith::flash_layout::kStagingSizeBytes) {
    return false;
  }

  const auto destination =
      static_cast<std::uint32_t>(midismith::flash_layout::kStagingAddress + offset_bytes);
  return InternalFlash::ProgramRegion(destination, word);
}

std::span<const std::uint8_t> StagingSlotFlash::Contents() const noexcept {
  return InternalFlash::ReadRegion(midismith::flash_layout::kStagingAddress,
                                   midismith::flash_layout::kStagingSizeBytes);
}

}  // namespace midismith::main_board::bsp::storage
