#include <catch2/catch_test_macros.hpp>
#include <telemetry/telemetry_sender_requirements.hpp>
#include <vector>

namespace midismith::telemetry {

class MockTelemetrySender : public TelemetrySenderRequirements {
 public:
  using TelemetrySenderRequirements::Send;

  std::size_t Send(std::span<const std::uint8_t> data) noexcept override {
    last_sent.assign(data.begin(), data.end());
    return data.size();
  }

  std::vector<std::uint8_t> last_sent;
};

TEST_CASE("TelemetrySenderRequirements can send raw spans", "[telemetry]") {
  MockTelemetrySender sender;
  std::uint8_t data[] = {0x01, 0x02, 0x03};

  sender.Send(std::span<const std::uint8_t>(data));

  REQUIRE(sender.last_sent.size() == 3);
  REQUIRE(sender.last_sent[0] == 0x01);
  REQUIRE(sender.last_sent[1] == 0x02);
  REQUIRE(sender.last_sent[2] == 0x03);
}

TEST_CASE("TelemetrySenderRequirements can send types via template", "[telemetry]") {
  MockTelemetrySender sender;
  std::uint32_t value = 0x12345678;

  sender.Send(value);

  REQUIRE(sender.last_sent.size() == sizeof(value));
  // Little endian check
  REQUIRE(sender.last_sent[0] == 0x78);
  REQUIRE(sender.last_sent[3] == 0x12);
}

}  // namespace midismith::telemetry
