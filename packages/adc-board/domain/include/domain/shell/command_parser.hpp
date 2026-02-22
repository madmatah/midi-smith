#pragma once

namespace midismith::adc_board::domain::shell {

class CommandParser {
 public:
  static int ParseInPlace(char* line, int max_args, char** argv_out) noexcept;
};

}  // namespace midismith::adc_board::domain::shell
