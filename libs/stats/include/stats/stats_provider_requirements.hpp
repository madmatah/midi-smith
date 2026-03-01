#pragma once

#include <string_view>

#include "stats/stats_publish_status.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace midismith::stats {

template <typename RequestT>
class StatsProviderRequirements {
 public:
  virtual ~StatsProviderRequirements() = default;

  virtual std::string_view Category() const noexcept = 0;
  virtual StatsPublishStatus ProvideStats(const RequestT& request,
                                          StatsVisitorRequirements& visitor) const noexcept = 0;
};

}  // namespace midismith::stats
