#pragma once

#include <cstdint>
#include <string_view>

namespace midismith::stats {

class StatsVisitorRequirements {
 public:
  virtual ~StatsVisitorRequirements() = default;

  virtual void OnMetric(std::string_view metric_name, std::uint32_t value) noexcept = 0;
  virtual void OnMetric(std::string_view metric_name, std::int32_t value) noexcept = 0;
  virtual void OnMetric(std::string_view metric_name, std::uint64_t value) noexcept = 0;
  virtual void OnMetric(std::string_view metric_name, std::int64_t value) noexcept = 0;
  virtual void OnMetric(std::string_view metric_name, bool value) noexcept = 0;
  virtual void OnMetric(std::string_view metric_name, std::string_view value_text) noexcept = 0;
};

}  // namespace midismith::stats
