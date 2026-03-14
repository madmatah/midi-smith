#include "app/storage/main_board_persistent_configuration.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

namespace midismith::main_board::app::storage {
namespace {

constexpr std::string_view kKeyCountKey = "key_count";
constexpr std::string_view kStartNoteKey = "start_note";
constexpr std::size_t kConfigKeyCount = 2u;

}  // namespace

MainBoardPersistentConfiguration::MainBoardPersistentConfiguration(
    midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept
    : storage_manager_(flash_storage,
                       midismith::main_board::domain::config::CreateDefaultMainBoardConfig()),
      ram_config_(midismith::main_board::domain::config::CreateDefaultMainBoardConfig()) {}

std::size_t MainBoardPersistentConfiguration::KeyCount() const noexcept {
  return kConfigKeyCount;
}

std::string_view MainBoardPersistentConfiguration::KeyAt(std::size_t index) const noexcept {
  switch (index) {
    case 0u:
      return kKeyCountKey;
    case 1u:
      return kStartNoteKey;
    default:
      return {};
  }
}

midismith::config::ConfigGetStatus MainBoardPersistentConfiguration::GetValue(
    std::string_view key, char* value_buffer, std::size_t value_buffer_size,
    std::size_t& value_length) const noexcept {
  value_length = 0u;

  std::uint32_t value_to_format = 0u;
  if (key == kKeyCountKey) {
    value_to_format = ram_config_.data.key_count;
  } else if (key == kStartNoteKey) {
    value_to_format = ram_config_.data.start_note;
  } else {
    return midismith::config::ConfigGetStatus::kUnknownKey;
  }

  if (value_buffer == nullptr || value_buffer_size == 0u) {
    return midismith::config::ConfigGetStatus::kBufferTooSmall;
  }

  auto result = std::to_chars(value_buffer, value_buffer + value_buffer_size, value_to_format);
  if (result.ec == std::errc::value_too_large) {
    return midismith::config::ConfigGetStatus::kBufferTooSmall;
  }
  if (result.ec != std::errc()) {
    return midismith::config::ConfigGetStatus::kUnavailable;
  }

  value_length = static_cast<std::size_t>(result.ptr - value_buffer);
  return midismith::config::ConfigGetStatus::kOk;
}

midismith::config::ConfigSetStatus MainBoardPersistentConfiguration::SetValue(
    std::string_view key, std::string_view value) noexcept {
  if (key != kKeyCountKey && key != kStartNoteKey) {
    return midismith::config::ConfigSetStatus::kUnknownKey;
  }
  if (value.empty()) {
    return midismith::config::ConfigSetStatus::kInvalidValue;
  }

  std::uint32_t parsed_value = 0u;
  const char* begin = value.data();
  const char* end = begin + value.size();
  auto parse_result = std::from_chars(begin, end, parsed_value);
  if (parse_result.ec != std::errc() || parse_result.ptr != end) {
    return midismith::config::ConfigSetStatus::kInvalidValue;
  }
  if (parsed_value > 255u) {
    return midismith::config::ConfigSetStatus::kInvalidValue;
  }

  if (key == kKeyCountKey) {
    ram_config_.data.key_count = static_cast<std::uint8_t>(parsed_value);
  } else {
    ram_config_.data.start_note = static_cast<std::uint8_t>(parsed_value);
  }

  return midismith::config::ConfigSetStatus::kOk;
}

midismith::config::ConfigStatus MainBoardPersistentConfiguration::Load() noexcept {
  return storage_manager_.Load(ram_config_);
}

midismith::config::TransactionResult MainBoardPersistentConfiguration::Commit() noexcept {
  auto save_result = storage_manager_.Save(ram_config_);
  if (save_result == midismith::bsp::storage::StorageOperationResult::kSuccess) {
    return midismith::config::TransactionResult::kSuccess;
  }
  return midismith::config::TransactionResult::kFailure;
}

bool MainBoardPersistentConfiguration::AddKeymapEntry(
    const midismith::main_board::domain::config::KeymapEntry& entry) noexcept {
  if (ram_config_.data.entry_count >= midismith::main_board::domain::config::kMaxKeymapEntries) {
    return false;
  }
  if (!midismith::main_board::domain::config::IsValidKeymapEntry(entry)) {
    return false;
  }
  ram_config_.data.entries[ram_config_.data.entry_count] = entry;
  ++ram_config_.data.entry_count;
  return true;
}

void MainBoardPersistentConfiguration::ResetKeymap(std::uint8_t key_count,
                                                   std::uint8_t start_note) noexcept {
  ram_config_.data.key_count = key_count;
  ram_config_.data.start_note = start_note;
  ram_config_.data.entry_count = 0;
}

const midismith::main_board::domain::config::MainBoardConfig&
MainBoardPersistentConfiguration::active_config() const noexcept {
  return ram_config_;
}

}  // namespace midismith::main_board::app::storage
