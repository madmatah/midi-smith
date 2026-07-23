#include "menu/line_buffer_stream.hpp"

namespace midismith::menu {

LineBufferStream::LineBufferStream(LineBuffer& buffer) noexcept : buffer_(buffer) {}

void LineBufferStream::Write(char character) noexcept {
  if (character == '\r') {
    buffer_.AppendChar('\n');
    previous_was_carriage_return_ = true;
    return;
  }
  if (character == '\n' && previous_was_carriage_return_) {
    previous_was_carriage_return_ = false;
    return;
  }
  previous_was_carriage_return_ = false;
  buffer_.AppendChar(character);
}

void LineBufferStream::Write(const char* text) noexcept {
  if (text == nullptr) {
    return;
  }
  while (*text != '\0') {
    Write(*text);
    text++;
  }
}

}  // namespace midismith::menu
