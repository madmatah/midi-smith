#include "bsp/storage/sd_card_image_source.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "bsp/memory_sections.hpp"
#include "bsp/storage/sd_card_bring_up.hpp"
#include "fatfs.h"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::main_board::bsp::storage {

namespace {

constexpr std::size_t kTransferBufferSizeBytes = 2048;
constexpr std::size_t kPathCapacityBytes = 64;

static_assert(kPathCapacityBytes > midismith::update_catalogue::kMaxImagePathLengthBytes,
              "this buffer must hold every path the image source contract admits, terminator "
              "included, or a legal path is silently answered as a missing file");

constexpr std::size_t kSdBlockSizeBytes = 512;

static_assert(kTransferBufferSizeBytes % kSdBlockSizeBytes == 0,
              "the SD driver programs the SDMMC1 IDMA straight at these buffers, block by block: "
              "they must live in the non-cacheable AXI window, never in the DTCM that .bss "
              "defaults to and that the IDMA cannot reach at all");

alignas(32) BSP_AXI_SRAM_NOCACHE FATFS file_system;
alignas(32) BSP_AXI_SRAM_NOCACHE FIL open_file;
alignas(32) BSP_AXI_SRAM_NOCACHE std::array<std::uint8_t, kTransferBufferSizeBytes> transfer_buffer;

void ForgetThatTheDriveWasEverInitialised() noexcept {
  FATFS_UnLinkDriver(SDPath);
  FATFS_LinkDriver(&SD_Driver, SDPath);
}

midismith::bsp::storage::VolumeMountResult TranslateMountResult(FRESULT result) noexcept {
  using midismith::bsp::storage::VolumeMountResult;
  switch (result) {
    case FR_OK:
      return VolumeMountResult::kMounted;
    case FR_NOT_READY:
      return VolumeMountResult::kDriveNotReady;
    case FR_NO_FILESYSTEM:
      return VolumeMountResult::kNoFileSystem;
    case FR_TIMEOUT:
      return VolumeMountResult::kVolumeLockTimedOut;
    case FR_INT_ERR:
      return VolumeMountResult::kFileSystemInternalError;
    default:
      return VolumeMountResult::kOtherFailure;
  }
}

bool CopyToNullTerminated(std::string_view path,
                          std::array<char, kPathCapacityBytes>& out) noexcept {
  if (path.empty() || path.size() >= out.size()) {
    return false;
  }
  std::copy_n(path.begin(), path.size(), out.begin());
  out[path.size()] = '\0';
  return true;
}

class OpenFileGuard {
 public:
  OpenFileGuard() noexcept = default;
  OpenFileGuard(const OpenFileGuard&) = delete;
  OpenFileGuard& operator=(const OpenFileGuard&) = delete;

  ~OpenFileGuard() {
    f_close(&open_file);
  }
};

}  // namespace

bool SdCardImageSource::Mount() noexcept {
  if (mounted_) {
    return true;
  }
  BeginSdCardBringUpAttempt();
  ForgetThatTheDriveWasEverInitialised();
  const FRESULT result = f_mount(&file_system, SDPath, 1);
  mount_result_ = TranslateMountResult(result);
  mounted_ = result == FR_OK;
  return mounted_;
}

midismith::bsp::storage::VolumeMountResult SdCardImageSource::last_mount_result() const noexcept {
  return mount_result_;
}

midismith::bsp::storage::SdCardBringUpOutcome SdCardImageSource::last_bring_up_outcome()
    const noexcept {
  return LastSdCardBringUpOutcome();
}

void SdCardImageSource::Unmount() noexcept {
  if (!mounted_) {
    return;
  }
  f_mount(nullptr, SDPath, 0);
  mounted_ = false;
}

std::optional<std::uint32_t> SdCardImageSource::SizeOf(std::string_view path) noexcept {
  std::array<char, kPathCapacityBytes> terminated_path{};
  if (!mounted_ || !CopyToNullTerminated(path, terminated_path)) {
    return std::nullopt;
  }

  FILINFO information{};
  if (f_stat(terminated_path.data(), &information) != FR_OK) {
    return std::nullopt;
  }

  return static_cast<std::uint32_t>(information.fsize);
}

std::optional<std::size_t> SdCardImageSource::ReadAt(std::string_view path,
                                                     std::uint32_t offset_bytes,
                                                     std::span<std::uint8_t> out) noexcept {
  std::array<char, kPathCapacityBytes> terminated_path{};
  if (!mounted_ || out.empty() || !CopyToNullTerminated(path, terminated_path)) {
    return std::nullopt;
  }

  if (f_open(&open_file, terminated_path.data(), FA_READ) != FR_OK) {
    return std::nullopt;
  }
  const OpenFileGuard guard;

  if (f_lseek(&open_file, offset_bytes) != FR_OK) {
    return std::nullopt;
  }

  std::size_t delivered_bytes = 0;
  while (delivered_bytes < out.size()) {
    const std::size_t wanted = std::min(out.size() - delivered_bytes, transfer_buffer.size());

    UINT read_bytes = 0;
    if (f_read(&open_file, transfer_buffer.data(), static_cast<UINT>(wanted), &read_bytes) !=
        FR_OK) {
      return std::nullopt;
    }
    if (read_bytes == 0) {
      break;
    }

    std::memcpy(out.data() + delivered_bytes, transfer_buffer.data(), read_bytes);
    delivered_bytes += read_bytes;
  }

  return delivered_bytes;
}

}  // namespace midismith::main_board::bsp::storage
