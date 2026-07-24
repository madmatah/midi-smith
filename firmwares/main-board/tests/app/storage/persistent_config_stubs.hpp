#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "app/storage/config_storage_control_requirements.hpp"
#include "bsp-types/storage/flash_sector_storage_requirements.hpp"
#include "os-types/mutex_requirements.hpp"

namespace midismith::main_board::test {

class ConfigStorageControlStub final
    : public midismith::main_board::app::storage::ConfigStorageControlRequirements {
 public:
  void RequestPersist() noexcept override {
    request_count++;
  }

  int request_count = 0;
};

class MutexStub final : public midismith::os::MutexRequirements {
 public:
  void Lock() noexcept override {}
  void Unlock() noexcept override {}
};

class FlashStorageStub final : public midismith::bsp::storage::FlashSectorStorageRequirements {
 public:
  static constexpr std::size_t kSectorSize = 4096;

  FlashStorageStub() noexcept {
    std::memset(storage_, 0xFF, sizeof(storage_));
  }

  std::size_t SectorSizeBytes() const noexcept override {
    return kSectorSize;
  }

  midismith::bsp::storage::StorageOperationResult Read(
      std::size_t offset_bytes, std::uint8_t* buffer,
      std::size_t length_bytes) const noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(buffer, storage_ + offset_bytes, length_bytes);
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult EraseSector() noexcept override {
    std::memset(storage_, 0xFF, sizeof(storage_));
    erase_count++;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  midismith::bsp::storage::StorageOperationResult Write(
      std::size_t offset_bytes, const std::uint8_t* data,
      std::size_t length_bytes) noexcept override {
    if (offset_bytes + length_bytes > kSectorSize) {
      return midismith::bsp::storage::StorageOperationResult::kError;
    }
    std::memcpy(storage_ + offset_bytes, data, length_bytes);
    write_count++;
    return midismith::bsp::storage::StorageOperationResult::kSuccess;
  }

  int erase_count = 0;
  int write_count = 0;

 private:
  alignas(32) std::uint8_t storage_[kSectorSize]{};
};

}  // namespace midismith::main_board::test
