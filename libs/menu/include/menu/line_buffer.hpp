#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace midismith::menu {

class LineBuffer {
 public:
  LineBuffer(char* text_storage, std::uint16_t* line_lengths, std::size_t max_lines,
             std::size_t line_capacity) noexcept;

  void Clear() noexcept;
  void Append(std::string_view text) noexcept;
  void AppendChar(char character) noexcept;

  std::string_view line(std::size_t index) const noexcept;
  std::size_t line_count() const noexcept;
  std::size_t max_lines() const noexcept;
  std::size_t line_capacity() const noexcept;

 private:
  void StartNextLine() noexcept;
  char* LineStart(std::size_t index) noexcept;
  const char* LineStart(std::size_t index) const noexcept;

  char* text_storage_;
  std::uint16_t* line_lengths_;
  std::size_t max_lines_;
  std::size_t line_capacity_;
  std::size_t line_count_ = 1;
};

}  // namespace midismith::menu
