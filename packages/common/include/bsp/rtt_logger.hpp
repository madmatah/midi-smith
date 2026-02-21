#pragma once

#include "app/logging/logger_requirements.hpp"

namespace bsp {

class RttLogger final : public app::Logging::LoggerRequirements {
 public:
  RttLogger() noexcept;

  void vlogf(app::Logging::Level level, const char* fmt, std::va_list* args) noexcept override;
};

}  // namespace bsp

namespace midismith::common::bsp {
using ::bsp::RttLogger;
}
