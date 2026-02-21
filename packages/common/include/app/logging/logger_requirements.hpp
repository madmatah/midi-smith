#pragma once

#include <cstdarg>

namespace app::Logging {

enum class Level : unsigned {
  Debug = 0,
  Info,
  Warn,
  Error,
};

class LoggerRequirements {
 public:
  virtual ~LoggerRequirements() = default;

  virtual void vlogf(Level level, const char* fmt, std::va_list* args) noexcept = 0;

  void logf(Level level, const char* fmt, ...) noexcept {
    std::va_list args;
    va_start(args, fmt);
    vlogf(level, fmt, &args);
    va_end(args);
  }

  void infof(const char* fmt, ...) noexcept {
    std::va_list args;
    va_start(args, fmt);
    vlogf(Level::Info, fmt, &args);
    va_end(args);
  }
};

}  // namespace app::Logging
