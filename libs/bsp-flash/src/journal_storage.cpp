#include "bsp-flash/journal_storage.hpp"

#include "bsp-flash/internal_flash.hpp"
#include "flash-layout/flash_layout.hpp"

namespace midismith::bsp_flash {

std::span<const std::uint8_t> JournalStorage::Sector() const noexcept {
  return InternalFlash::ReadRegion(midismith::flash_layout::kBootJournalAddress,
                                   midismith::flash_layout::kBootJournalSizeBytes);
}

bool JournalStorage::ProgramRecord(std::size_t offset_bytes,
                                   std::span<const std::uint8_t> record) noexcept {
  if (offset_bytes + record.size() > midismith::flash_layout::kBootJournalSizeBytes) {
    return false;
  }

  const auto destination =
      static_cast<std::uint32_t>(midismith::flash_layout::kBootJournalAddress + offset_bytes);
  return InternalFlash::ProgramRegion(destination, record);
}

bool JournalStorage::EraseSector() noexcept {
  return InternalFlash::EraseRegion(midismith::flash_layout::kBootJournalAddress,
                                    midismith::flash_layout::kBootJournalSizeBytes);
}

}  // namespace midismith::bsp_flash
