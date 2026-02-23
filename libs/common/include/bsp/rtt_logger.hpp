#pragma once

#include "logging/logger_requirements.hpp"

namespace midismith::common::bsp {

class RttLogger final : public midismith::logging::LoggerRequirements {
 public:
  RttLogger() noexcept;

  void vlogf(midismith::logging::Level level, const char* fmt,
             std::va_list* args) noexcept override;
};

}  // namespace midismith::common::bsp
