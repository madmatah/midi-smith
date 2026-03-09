#if defined(UNIT_TESTS)
#include "stats/stats_provider_requirements.hpp"

#include <fakeit.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <string_view>

#include "stats/empty_stats_request.hpp"

namespace {

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
using fakeit::Fake;

#define fakeit_Method(mock, method) Method(mock, method)

}  // namespace

TEST_CASE("The stats provider contract", "[libs][stats]") {
  Mock<midismith::stats::StatsProviderRequirements<midismith::stats::EmptyStatsRequest>>
      provider_mock;
  Mock<midismith::stats::StatsVisitorRequirements> visitor_mock;
  midismith::stats::EmptyStatsRequest request;

  When(fakeit_Method(provider_mock, Category)).Return("test");
  When(fakeit_Method(provider_mock, ProvideStats))
      .Return(midismith::stats::StatsPublishStatus::kOk);

  REQUIRE(provider_mock.get().Category() == "test");
  REQUIRE(provider_mock.get().ProvideStats(request, visitor_mock.get()) ==
          midismith::stats::StatsPublishStatus::kOk);

  Verify(fakeit_Method(provider_mock, Category)).Once();
  Verify(fakeit_Method(provider_mock, ProvideStats)).Once();
}
#endif
