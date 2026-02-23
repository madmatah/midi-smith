#include "app/storage/adc_board_persistent_configuration.hpp"

#include <charconv>
#include <cstdint>
#include <system_error>

namespace midismith::adc_board::app::storage {
namespace {

constexpr std::string_view kCanBoardIdKey = "can_board_id";

}  // namespace

AdcBoardPersistentConfiguration::AdcBoardPersistentConfiguration(
    midismith::bsp::storage::FlashSectorStorageRequirements& flash_storage) noexcept
    : storage_manager_(flash_storage,
                       midismith::adc_board::domain::config::CreateDefaultAdcBoardConfig()),
      ram_config_(midismith::adc_board::domain::config::CreateDefaultAdcBoardConfig()) {}

std::size_t AdcBoardPersistentConfiguration::KeyCount() const noexcept {
  return 1u;
}

std::string_view AdcBoardPersistentConfiguration::KeyAt(std::size_t index) const noexcept {
  if (index == 0u) {
    return kCanBoardIdKey;
  }
  return {};
}

midismith::config::ConfigGetStatus AdcBoardPersistentConfiguration::GetValue(
    std::string_view key, char* value_buffer, std::size_t value_buffer_size,
    std::size_t& value_length) const noexcept {
  value_length = 0u;
  if (key != kCanBoardIdKey) {
    return midismith::config::ConfigGetStatus::kUnknownKey;
  }
  if (value_buffer == nullptr || value_buffer_size == 0u) {
    return midismith::config::ConfigGetStatus::kBufferTooSmall;
  }

  auto result = std::to_chars(value_buffer, value_buffer + value_buffer_size,
                              static_cast<std::uint32_t>(ram_config_.data.can_board_id));
  if (result.ec == std::errc::value_too_large) {
    return midismith::config::ConfigGetStatus::kBufferTooSmall;
  }
  if (result.ec != std::errc()) {
    return midismith::config::ConfigGetStatus::kUnavailable;
  }

  value_length = static_cast<std::size_t>(result.ptr - value_buffer);
  return midismith::config::ConfigGetStatus::kOk;
}

midismith::config::ConfigSetStatus AdcBoardPersistentConfiguration::SetValue(
    std::string_view key, std::string_view value) noexcept {
  if (key != kCanBoardIdKey) {
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
  if (!UpdateBoardId(static_cast<std::uint8_t>(parsed_value))) {
    return midismith::config::ConfigSetStatus::kInvalidValue;
  }

  return midismith::config::ConfigSetStatus::kOk;
}

midismith::config::ConfigStatus AdcBoardPersistentConfiguration::Load() noexcept {
  auto status = storage_manager_.Load(ram_config_);
  if (status == midismith::config::ConfigStatus::kOlderVersion) {
    ram_config_ = midismith::adc_board::domain::config::MigrateAdcBoardConfig(ram_config_,
                                                                              ram_config_.version);
  }
  return status;
}

bool AdcBoardPersistentConfiguration::UpdateBoardId(std::uint8_t board_id) noexcept {
  if (!midismith::adc_board::domain::config::IsValidBoardId(board_id)) {
    return false;
  }

  ram_config_.data.can_board_id = board_id;
  return true;
}

midismith::config::TransactionResult AdcBoardPersistentConfiguration::Commit() noexcept {
  auto save_result = storage_manager_.Save(ram_config_);
  if (save_result == midismith::bsp::storage::StorageOperationResult::kSuccess) {
    return midismith::config::TransactionResult::kSuccess;
  }
  return midismith::config::TransactionResult::kFailure;
}

const midismith::adc_board::domain::config::AdcBoardConfig&
AdcBoardPersistentConfiguration::active_config() const noexcept {
  return ram_config_;
}

}  // namespace midismith::adc_board::app::storage
