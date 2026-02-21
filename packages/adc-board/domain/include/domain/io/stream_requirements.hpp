#pragma once

#include <cstdint>
#include <string_view>

namespace domain::io {

enum class ReadResult {
  kOk,
  kNoData,
  kError,
};

class WritableStreamRequirements {
 public:
  virtual ~WritableStreamRequirements() = default;

  virtual void Write(char c) noexcept = 0;
  virtual void Write(const char* str) noexcept = 0;

  void Write(std::string_view str) noexcept {
    for (char c : str) {
      Write(c);
    }
  }
};

class ReadableStreamRequirements {
 public:
  virtual ~ReadableStreamRequirements() = default;

  virtual ReadResult Read(std::uint8_t& byte) noexcept = 0;
};

class StreamRequirements : public WritableStreamRequirements, public ReadableStreamRequirements {
 public:
  virtual ~StreamRequirements() = default;
};

}  // namespace domain::io
