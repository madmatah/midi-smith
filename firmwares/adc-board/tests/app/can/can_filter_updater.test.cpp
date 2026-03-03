#if defined(UNIT_TESTS)

#include "app/can/can_filter_updater.hpp"

#include <catch2/catch_test_macros.hpp>
#include <span>
#include <vector>

#include "bsp-types/can/fdcan_filter_config.hpp"
#include "protocol-can/can_filter_factory.hpp"

namespace {

struct CallLog {
  enum class Event { kStop, kConfigureReceiveFilters, kConfigureGlobalRejectFilter, kStart };
  std::vector<Event> events;
};

class TransceiverAdminStub final : public midismith::bsp::can::FdcanTransceiverAdminRequirements {
 public:
  explicit TransceiverAdminStub(CallLog& log) noexcept : log_(log) {}

  bool Stop() noexcept override {
    log_.events.push_back(CallLog::Event::kStop);
    return true;
  }

  bool ConfigureReceiveFilters(
      std::span<const midismith::bsp::can::FdcanFilterConfig> configs) noexcept override {
    log_.events.push_back(CallLog::Event::kConfigureReceiveFilters);
    last_filters_.assign(configs.begin(), configs.end());
    return true;
  }

  bool ConfigureGlobalRejectFilter() noexcept override {
    log_.events.push_back(CallLog::Event::kConfigureGlobalRejectFilter);
    return true;
  }

  bool Start() noexcept override {
    log_.events.push_back(CallLog::Event::kStart);
    return true;
  }

  const std::vector<midismith::bsp::can::FdcanFilterConfig>& last_filters() const noexcept {
    return last_filters_;
  }

 private:
  CallLog& log_;
  std::vector<midismith::bsp::can::FdcanFilterConfig> last_filters_;
};

}  // namespace

TEST_CASE("The CanFilterUpdater class") {
  CallLog log;
  TransceiverAdminStub transceiver_admin(log);
  midismith::adc_board::app::can::CanFilterUpdater manager(transceiver_admin);

  SECTION("The OnCanNodeIdChanged() method") {
    SECTION("Should call stop, configure filters, configure global reject, then start in order") {
      manager.OnCanNodeIdChanged(3);

      REQUIRE(log.events.size() == 4);
      REQUIRE(log.events[0] == CallLog::Event::kStop);
      REQUIRE(log.events[1] == CallLog::Event::kConfigureReceiveFilters);
      REQUIRE(log.events[2] == CallLog::Event::kConfigureGlobalRejectFilter);
      REQUIRE(log.events[3] == CallLog::Event::kStart);
    }

    SECTION("Should apply the filters produced by CanFilterFactory for the given node ID") {
      constexpr std::uint8_t kNodeId = 5;
      manager.OnCanNodeIdChanged(kNodeId);

      const auto expected = midismith::protocol_can::CanFilterFactory::MakeAdcFilters(kNodeId);
      const auto& actual = transceiver_admin.last_filters();

      REQUIRE(actual.size() == expected.filters.size());
      for (std::size_t i = 0; i < expected.filters.size(); ++i) {
        REQUIRE(actual[i].filter_index == expected.filters[i].filter_index);
        REQUIRE(actual[i].id == expected.filters[i].id);
        REQUIRE(actual[i].id_mask == expected.filters[i].id_mask);
      }
    }

    SECTION("Should apply distinct filters for different node IDs") {
      manager.OnCanNodeIdChanged(1);
      const auto filters_node1 = transceiver_admin.last_filters();

      manager.OnCanNodeIdChanged(8);
      const auto filters_node8 = transceiver_admin.last_filters();

      REQUIRE(filters_node1[1].id != filters_node8[1].id);
    }
  }
}

#endif  // UNIT_TESTS
