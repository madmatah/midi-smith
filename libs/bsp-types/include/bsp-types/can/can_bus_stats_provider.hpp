#pragma once

#include <cstdint>
#include <string_view>

#include "bsp-types/can/can_bus_stats_requirements.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"

namespace midismith::bsp::can {

class CanBusStatsProvider final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  explicit CanBusStatsProvider(midismith::bsp::can::CanBusStatsRequirements& stats) noexcept
      : stats_(stats) {}

  std::string_view Category() const noexcept override {
    return "can";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    const midismith::bsp::can::CanBusStatsSnapshot snapshot = stats_.CaptureSnapshot();
    visitor.OnMetric("tx_frames_sent", snapshot.tx_frames_sent);
    visitor.OnMetric("tx_frames_failed", snapshot.tx_frames_failed);
    visitor.OnMetric("rx_frames_received", snapshot.rx_frames_received);
    visitor.OnMetric("rx_queue_overflows", snapshot.rx_queue_overflows);
    visitor.OnMetric("bus_off", snapshot.bus_off);
    visitor.OnMetric("error_passive", snapshot.error_passive);
    visitor.OnMetric("warning", snapshot.warning);
    visitor.OnMetric("last_error_code", LastErrorCodeLabel(snapshot.last_error_code));
    visitor.OnMetric("transmit_error_count",
                     static_cast<std::uint32_t>(snapshot.transmit_error_count));
    visitor.OnMetric("receive_error_count",
                     static_cast<std::uint32_t>(snapshot.receive_error_count));
    return midismith::stats::StatsPublishStatus::kOk;
  }

 private:
  static std::string_view LastErrorCodeLabel(std::uint8_t code) noexcept {
    constexpr std::string_view kLabels[] = {"none", "stuff", "form", "ack",
                                            "bit1", "bit0",  "crc",  "no_change"};
    if (code < 8u) {
      return kLabels[code];
    }
    return "unknown";
  }

  midismith::bsp::can::CanBusStatsRequirements& stats_;
};

}  // namespace midismith::bsp::can
