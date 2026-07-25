#include "bsp/storage/sd_card_image_source.hpp"

#include <algorithm>
#include <array>
#include <cstring>

#include "bsp/memory_sections.hpp"
#include "bsp_driver_sd.h"
#include "fatfs.h"

namespace midismith::main_board::bsp::storage {

namespace {

constexpr std::size_t kTransferBufferSizeBytes = 2048;
constexpr std::size_t kPathCapacity = 64;

constexpr std::size_t kSdBlockSizeBytes = 512;

static_assert(kTransferBufferSizeBytes % kSdBlockSizeBytes == 0,
              "the SD driver programs the SDMMC1 IDMA straight at these buffers, block by block: "
              "they must live in the non-cacheable AXI window, never in the DTCM that .bss "
              "defaults to and that the IDMA cannot reach at all");

alignas(32) BSP_AXI_SRAM_NOCACHE FATFS file_system;
alignas(32) BSP_AXI_SRAM_NOCACHE FIL open_file;
alignas(32) BSP_AXI_SRAM_NOCACHE std::array<std::uint8_t, kTransferBufferSizeBytes> transfer_buffer;

bool CopyToNullTerminated(std::string_view path, std::array<char, kPathCapacity>& out) noexcept {
  if (path.empty() || path.size() >= out.size()) {
    return false;
  }
  std::copy_n(path.begin(), path.size(), out.begin());
  out[path.size()] = '\0';
  return true;
}

class OpenFileGuard {
 public:
  explicit OpenFileGuard(bool opened) noexcept : opened_(opened) {}
  OpenFileGuard(const OpenFileGuard&) = delete;
  OpenFileGuard& operator=(const OpenFileGuard&) = delete;

  ~OpenFileGuard() {
    if (opened_) {
      f_close(&open_file);
    }
  }

 private:
  bool opened_;
};

}  // namespace

bool SdCardImageSource::IsCardPresent() const noexcept {
  return BSP_SD_IsDetected() == SD_PRESENT;
}

bool SdCardImageSource::Mount() noexcept {
  if (mounted_) {
    return true;
  }
  if (!IsCardPresent()) {
    return false;
  }
  mounted_ = f_mount(&file_system, SDPath, 1) == FR_OK;
  return mounted_;
}

void SdCardImageSource::Unmount() noexcept {
  if (!mounted_) {
    return;
  }
  f_mount(nullptr, SDPath, 0);
  mounted_ = false;
}

std::optional<std::uint32_t> SdCardImageSource::SizeOf(std::string_view path) noexcept {
  std::array<char, kPathCapacity> terminated_path{};
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
  std::array<char, kPathCapacity> terminated_path{};
  if (!mounted_ || out.empty() || !CopyToNullTerminated(path, terminated_path)) {
    return std::nullopt;
  }

  if (f_open(&open_file, terminated_path.data(), FA_READ) != FR_OK) {
    return std::nullopt;
  }
  const OpenFileGuard guard{true};

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
