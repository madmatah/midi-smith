#include <catch2/catch_test_macros.hpp>
#include <fakeit.hpp>

#include "domain/math.hpp"

namespace {

struct ValueRequirements {
  virtual ~ValueRequirements() = default;
  virtual int get() noexcept = 0;
};

}  // namespace

TEST_CASE("domain add") {
  REQUIRE(domain::add(2, 3) == 5);
}

TEST_CASE("fakeit mock") {
  using fakeit::Mock;
  using fakeit::Verify;
  using fakeit::When;

  Mock<ValueRequirements> mock;
  When(Method(mock, get)).Return(7);

  ValueRequirements& v = mock.get();
  REQUIRE(v.get() == 7);

  Verify(Method(mock, get)).Once();
}
