#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "product-id/product_id.hpp"

namespace midismith::boot_control {

inline constexpr std::size_t kBootJournalRecordSizeBytes = 32;
inline constexpr std::array<std::uint8_t, 4> kBootJournalMagic = {'M', 'S', 'B', 'C'};
inline constexpr std::uint8_t kErasedFlashByte = 0xFF;

static_assert(kBootJournalRecordSizeBytes == 32,
              "a record is one STM32H7 flash word: the smallest unit that can be programmed, and "
              "one that ECC forbids programming a second time between erases");

enum class UpdateState : std::uint8_t {
  kIdle = 0x00,
  kUpdatePending = 0x01,
  kUpdateInProgress = 0x02,
  kUpdateFailed = 0x03,
};

[[nodiscard]] constexpr bool IsKnownUpdateState(std::uint8_t raw_value) noexcept {
  switch (static_cast<UpdateState>(raw_value)) {
    case UpdateState::kIdle:
    case UpdateState::kUpdatePending:
    case UpdateState::kUpdateInProgress:
    case UpdateState::kUpdateFailed:
      return true;
    default:
      return false;
  }
}

struct BootJournalRecord {
  std::uint32_t sequence_number = 0;
  UpdateState state = UpdateState::kIdle;
  std::uint32_t staged_payload_crc32 = 0;
  std::uint32_t staged_payload_size_bytes = 0;
  product_id::ProductId staged_product_id = product_id::ProductId::kUnknown;

  bool operator==(const BootJournalRecord&) const = default;

  [[nodiscard]] std::optional<std::size_t> Serialize(
      std::span<std::uint8_t> out_buffer) const noexcept;

  [[nodiscard]] static std::optional<BootJournalRecord> Deserialize(
      std::span<const std::uint8_t> buffer) noexcept;
};

[[nodiscard]] bool IsErasedRecordSlot(std::span<const std::uint8_t> slot) noexcept;

}  // namespace midismith::boot_control
