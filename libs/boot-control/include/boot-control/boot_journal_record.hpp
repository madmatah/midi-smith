#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

#include "firmware-image/product_id.hpp"

namespace midismith::boot_control {

inline constexpr std::size_t kBootJournalRecordSizeBytes = 32;
inline constexpr std::array<std::uint8_t, 4> kBootJournalMagic = {'M', 'S', 'B', 'C'};
inline constexpr std::uint8_t kErasedFlashByte = 0xFF;

enum class UpdateState : std::uint8_t {
  kIdle = 0x00,
  kUpdatePending = 0x01,
  kUpdateInProgress = 0x02,
  kUpdateFailed = 0x03,
};

[[nodiscard]] constexpr bool IsKnownUpdateState(std::uint8_t raw_value) noexcept {
  return raw_value <= static_cast<std::uint8_t>(UpdateState::kUpdateFailed);
}

struct BootJournalRecord {
  std::uint32_t sequence_number = 0;
  UpdateState state = UpdateState::kIdle;
  std::uint32_t staged_payload_crc32 = 0;
  std::uint32_t staged_payload_size_bytes = 0;
  firmware_image::ProductId staged_product_id = firmware_image::ProductId::kUnknown;

  bool operator==(const BootJournalRecord&) const = default;

  [[nodiscard]] std::optional<std::size_t> Serialize(
      std::span<std::uint8_t> out_buffer) const noexcept;

  [[nodiscard]] static std::optional<BootJournalRecord> Deserialize(
      std::span<const std::uint8_t> buffer) noexcept;
};

[[nodiscard]] bool IsErasedRecordSlot(std::span<const std::uint8_t> slot) noexcept;

}  // namespace midismith::boot_control
