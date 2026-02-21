#include "app/analog/acquisition_sequencer.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <vector>

namespace {

enum class Call : std::uint8_t {
  kTiaSet = 0,
  kDelayUs = 1,
  kAdcStart = 2,
  kTiaReset = 3,
  kAdcStop = 4,
};

class FakeGpio final : public bsp::GpioRequirements {
 public:
  explicit FakeGpio(std::vector<Call>& calls) noexcept : calls_(calls) {}

  void set() noexcept override {
    calls_.push_back(Call::kTiaSet);
  }
  void reset() noexcept override {
    calls_.push_back(Call::kTiaReset);
  }
  void toggle() noexcept override {}
  bool read() const noexcept override {
    return false;
  }

 private:
  std::vector<Call>& calls_;
};

class FakeDelay final : public app::analog::DelayRequirements {
 public:
  explicit FakeDelay(std::vector<Call>& calls) noexcept : calls_(calls) {}

  void DelayUs(std::uint32_t delay_us) noexcept override {
    delay_us_ = delay_us;
    calls_.push_back(Call::kDelayUs);
  }

  std::uint32_t delay_us() const noexcept {
    return delay_us_;
  }

 private:
  std::vector<Call>& calls_;
  std::uint32_t delay_us_ = 0;
};

class FakeAdcDma final : public app::analog::AdcDmaControlRequirements {
 public:
  FakeAdcDma(std::vector<Call>& calls, bool start_result) noexcept
      : calls_(calls), start_result_(start_result) {}

  bool Start() noexcept override {
    calls_.push_back(Call::kAdcStart);
    return start_result_;
  }
  void Stop() noexcept override {
    calls_.push_back(Call::kAdcStop);
  }

 private:
  std::vector<Call>& calls_;
  bool start_result_ = true;
};

}  // namespace

TEST_CASE("The AcquisitionSequencer class") {
  SECTION("The Enable() method") {
    SECTION("When called with settle_us=70") {
      SECTION("Should enable TIA, wait, then start ADC/DMA") {
        std::vector<Call> calls;
        FakeGpio gpio(calls);
        FakeDelay delay(calls);
        FakeAdcDma adc(calls, true);
        app::analog::AcquisitionSequencer sequencer(gpio, delay, adc);

        const bool ok = sequencer.Enable(70);

        REQUIRE(ok);
        REQUIRE(delay.delay_us() == 70);
        REQUIRE(calls == std::vector<Call>{Call::kTiaSet, Call::kDelayUs, Call::kAdcStart});
      }
    }
  }

  SECTION("The Disable() method") {
    SECTION("When called") {
      SECTION("Should disable TIA, then stop ADC/DMA") {
        std::vector<Call> calls;
        FakeGpio gpio(calls);
        FakeDelay delay(calls);
        FakeAdcDma adc(calls, true);
        app::analog::AcquisitionSequencer sequencer(gpio, delay, adc);

        sequencer.Disable();

        REQUIRE(calls == std::vector<Call>{Call::kTiaReset, Call::kAdcStop});
      }
    }
  }
}
