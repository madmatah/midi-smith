#if defined(UNIT_TESTS)
#include "stats/stats_provider_requirements.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "stats/empty_stats_request.hpp"

namespace {

class VisitorStub final : public midismith::stats::StatsVisitorRequirements {
 public:
  void OnMetric(std::string_view, std::uint32_t) noexcept override {}
  void OnMetric(std::string_view, std::int32_t) noexcept override {}
  void OnMetric(std::string_view, std::uint64_t) noexcept override {}
  void OnMetric(std::string_view, std::int64_t) noexcept override {}
  void OnMetric(std::string_view, bool) noexcept override {}
  void OnMetric(std::string_view, std::string_view) noexcept override {}
};

class ProviderStub final
    : public midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest> {
 public:
  std::string_view Category() const noexcept override {
    return "test";
  }

  midismith::stats::StatsPublishStatus ProvideStats(
      const midismith::stats::EmptyStatsRequest&,
      midismith::stats::StatsVisitorRequirements&) const noexcept override {
    return midismith::stats::StatsPublishStatus::kOk;
  }
};

}  // namespace

TEST_CASE("The stats provider contract", "[libs][stats]") {
  ProviderStub provider;
  VisitorStub visitor;
  midismith::stats::EmptyStatsRequest request;

  REQUIRE(provider.Category() == "test");
  REQUIRE(provider.ProvideStats(request, visitor) == midismith::stats::StatsPublishStatus::kOk);
}
#endif
