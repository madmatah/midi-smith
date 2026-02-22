#pragma once

#include "app/logging/logger_requirements.hpp"

namespace midismith::common::bsp {

class RttLogger final : public midismith::common::app::logging::LoggerRequirements {
 public:
  RttLogger() noexcept;

  void vlogf(midismith::common::app::logging::Level level, const char* fmt,
             std::va_list* args) noexcept override;
};

}  // namespace midismith::common::bsp
