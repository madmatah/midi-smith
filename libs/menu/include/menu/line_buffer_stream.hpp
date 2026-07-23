#pragma once

#include "io/stream_requirements.hpp"
#include "menu/line_buffer.hpp"

namespace midismith::menu {

class LineBufferStream final : public midismith::io::WritableStreamRequirements {
 public:
  explicit LineBufferStream(LineBuffer& buffer) noexcept;

  void Write(char character) noexcept override;
  void Write(const char* text) noexcept override;

 private:
  LineBuffer& buffer_;
  bool previous_was_carriage_return_ = false;
};

}  // namespace midismith::menu
