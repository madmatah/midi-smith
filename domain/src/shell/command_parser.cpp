#include "domain/shell/command_parser.hpp"

namespace domain::shell {

int CommandParser::ParseInPlace(char* line, int max_args, char** argv_out) noexcept {
  if (line == nullptr || argv_out == nullptr || max_args <= 0) {
    return 0;
  }

  int argc = 0;
  bool in_token = false;

  for (char* p = line; *p != '\0'; ++p) {
    if (*p == ' ' || *p == '\t') {
      if (in_token) {
        *p = '\0';
        in_token = false;
      }
    } else {
      if (!in_token) {
        if (argc >= max_args) {
          return -1;
        }
        argv_out[argc++] = p;
        in_token = true;
      }
    }
  }

  return argc;
}

}  // namespace domain::shell
