#pragma once

namespace domain::shell {

class CommandParser {
 public:
  static int ParseInPlace(char* line, int max_args, char** argv_out) noexcept;
};

}  // namespace domain::shell
