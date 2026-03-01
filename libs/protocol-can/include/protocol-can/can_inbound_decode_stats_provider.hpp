#pragma once

#include <string_view>

#include "protocol-can/can_inbound_decode_stats_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"
#include "stats/stats_publish_status.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace midismith::protocol_can {

class CanInboundDecodeStatsProvider final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  explicit CanInboundDecodeStatsProvider(CanInboundDecodeStatsRequirements& stats) noexcept
      : stats_(stats) {}

  std::string_view Category() const noexcept override {
    return "can_inbound";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    const CanInboundDecodeStats snapshot = stats_.CaptureDecodeStats();
    visitor.OnMetric("dispatched_message_count", snapshot.dispatched_message_count);
    visitor.OnMetric("unknown_identifier_count", snapshot.unknown_identifier_count);
    visitor.OnMetric("invalid_payload_count", snapshot.invalid_payload_count);
    visitor.OnMetric("dropped_message_count", snapshot.dropped_message_count);
    return midismith::stats::StatsPublishStatus::kOk;
  }

 private:
  CanInboundDecodeStatsRequirements& stats_;
};

}  // namespace midismith::protocol_can
