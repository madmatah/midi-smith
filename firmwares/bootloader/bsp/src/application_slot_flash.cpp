#include "bsp/application_slot_flash.hpp"

#include "bsp-flash/internal_flash.hpp"
#include "flash-layout/flash_layout.hpp"

namespace midismith::bootloader::bsp {

std::span<const std::uint8_t> ApplicationSlotFlash::StagedContainer() const noexcept {
  return midismith::bsp_flash::InternalFlash::ReadRegion(
      midismith::flash_layout::kStagingAddress, midismith::flash_layout::kStagingSizeBytes);
}

std::span<const std::uint8_t> ApplicationSlotFlash::ApplicationSlot() const noexcept {
  return midismith::bsp_flash::InternalFlash::ReadRegion(
      midismith::flash_layout::kApplicationLoadAddress,
      midismith::flash_layout::kApplicationSlotSizeBytes);
}

bool ApplicationSlotFlash::EraseApplicationSlot(std::size_t length_bytes) noexcept {
  return midismith::bsp_flash::InternalFlash::EraseRegion(
      midismith::flash_layout::kApplicationLoadAddress, length_bytes);
}

bool ApplicationSlotFlash::ProgramApplicationSlot(std::span<const std::uint8_t> payload) noexcept {
  return midismith::bsp_flash::InternalFlash::ProgramRegion(
      midismith::flash_layout::kApplicationLoadAddress, payload);
}

}  // namespace midismith::bootloader::bsp
