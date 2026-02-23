#pragma once

#include <cstddef>
#include <string_view>

namespace midismith::config {

enum class ConfigGetStatus {
  kOk,
  kUnknownKey,
  kBufferTooSmall,
  kUnavailable,
};

enum class ConfigSetStatus {
  kOk,
  kUnknownKey,
  kInvalidValue,
};

enum class TransactionResult {
  kSuccess,
  kFailure,
};

class TransactionalConfigDictionary {
 public:
  virtual ~TransactionalConfigDictionary() = default;

  virtual std::size_t KeyCount() const noexcept = 0;
  virtual std::string_view KeyAt(std::size_t index) const noexcept = 0;
  virtual ConfigGetStatus GetValue(std::string_view key, char* value_buffer,
                                   std::size_t value_buffer_size,
                                   std::size_t& value_length) const noexcept = 0;
  virtual ConfigSetStatus SetValue(std::string_view key, std::string_view value) noexcept = 0;
  virtual TransactionResult Commit() noexcept = 0;
};

}  // namespace midismith::config
