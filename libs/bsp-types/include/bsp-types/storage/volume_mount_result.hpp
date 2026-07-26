#pragma once

#include <cstdint>

namespace midismith::bsp::storage {

enum class VolumeMountResult : std::uint8_t {
  kNotAttempted = 0,
  kMounted,
  kDriveNotReady,
  kNoFileSystem,
  kVolumeLockTimedOut,
  kFileSystemInternalError,
  kOtherFailure,
};

}  // namespace midismith::bsp::storage
