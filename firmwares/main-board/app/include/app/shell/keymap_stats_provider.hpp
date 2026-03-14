#pragma once

#include <cstdio>
#include <string_view>

#include "app/keymap/keymap_setup_coordinator.hpp"
#include "stats/empty_stats_request.hpp"
#include "stats/stats_provider_requirements.hpp"
#include "stats/stats_visitor_requirements.hpp"

namespace midismith::main_board::app::shell {

class KeymapStatsProvider final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  explicit KeymapStatsProvider(
      midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator) noexcept
      : coordinator_(coordinator) {}

  std::string_view Category() const noexcept override {
    return "keymap";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements& visitor) const noexcept override {
    const auto& data = coordinator_.persistent_config().active_config().data;

    visitor.OnMetric("status", coordinator_.is_in_progress() ? std::string_view{"in_progress"}
                                                             : std::string_view{"saved"});
    visitor.OnMetric("start_note", static_cast<std::uint32_t>(data.start_note));
    visitor.OnMetric("key_count", static_cast<std::uint32_t>(data.key_count));
    visitor.OnMetric("entry_count", static_cast<std::uint32_t>(data.entry_count));

    for (std::uint8_t i = 0u; i < data.entry_count; ++i) {
      const auto& entry = data.entries[i];
      char name_buf[16];
      const int written =
          std::snprintf(name_buf, sizeof(name_buf), "board%u#%u", entry.board_id, entry.sensor_id);
      if (written > 0) {
        visitor.OnMetric(std::string_view(name_buf, static_cast<std::size_t>(written)),
                         static_cast<std::uint32_t>(entry.midi_note));
      }
    }

    return midismith::stats::StatsPublishStatus::kOk;
  }

 private:
  midismith::main_board::app::keymap::KeymapSetupCoordinator& coordinator_;
};

}  // namespace midismith::main_board::app::shell
