#include "menu/line_buffer.hpp"

namespace midismith::menu {

LineBuffer::LineBuffer(char* text_storage, std::uint16_t* line_lengths, std::size_t max_lines,
                       std::size_t line_capacity) noexcept
    : text_storage_(text_storage),
      line_lengths_(line_lengths),
      max_lines_(max_lines),
      line_capacity_(line_capacity),
      line_count_(max_lines == 0 ? 0 : 1) {
  Clear();
}

void LineBuffer::Clear() noexcept {
  for (std::size_t line_index = 0; line_index < max_lines_; line_index++) {
    line_lengths_[line_index] = 0;
    if (line_capacity_ > 0) {
      LineStart(line_index)[0] = '\0';
    }
  }
  line_count_ = max_lines_ == 0 ? 0 : 1;
}

void LineBuffer::Append(std::string_view text) noexcept {
  for (char character : text) {
    AppendChar(character);
  }
}

void LineBuffer::AppendChar(char character) noexcept {
  if (line_count_ == 0 || line_capacity_ == 0) {
    return;
  }
  if (character == '\r') {
    return;
  }
  if (character == '\n') {
    StartNextLine();
    return;
  }
  const std::size_t current_line = line_count_ - 1;
  if (line_lengths_[current_line] >= line_capacity_ - 1) {
    StartNextLine();
  }
  const std::size_t writable_line = line_count_ - 1;
  if (line_lengths_[writable_line] >= line_capacity_ - 1) {
    return;
  }
  char* line_text = LineStart(writable_line);
  line_text[line_lengths_[writable_line]] = character;
  line_lengths_[writable_line]++;
  line_text[line_lengths_[writable_line]] = '\0';
}

std::string_view LineBuffer::line(std::size_t index) const noexcept {
  if (index >= line_count_) {
    return {};
  }
  return {LineStart(index), line_lengths_[index]};
}

std::size_t LineBuffer::line_count() const noexcept {
  return line_count_;
}

std::size_t LineBuffer::max_lines() const noexcept {
  return max_lines_;
}

std::size_t LineBuffer::line_capacity() const noexcept {
  return line_capacity_;
}

void LineBuffer::StartNextLine() noexcept {
  if (line_count_ >= max_lines_) {
    return;
  }
  line_count_++;
  line_lengths_[line_count_ - 1] = 0;
  LineStart(line_count_ - 1)[0] = '\0';
}

char* LineBuffer::LineStart(std::size_t index) noexcept {
  return text_storage_ + (index * line_capacity_);
}

const char* LineBuffer::LineStart(std::size_t index) const noexcept {
  return text_storage_ + (index * line_capacity_);
}

}  // namespace midismith::menu
