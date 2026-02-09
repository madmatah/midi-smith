#if defined(UNIT_TESTS)

#include "app/analog/adc_rank_mapped_frame_decoder.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

#include "app/config/sensors.hpp"
#include "domain/sensors/sensor_state.hpp"

TEST_CASE("The AdcRankMappedFrameDecoder class") {
  SECTION("The ApplySequence() method") {
    class TestSensorGroup {
     public:
      TestSensorGroup(domain::sensors::SensorState* sensors, std::size_t sensor_count) noexcept
          : sensors_(sensors), sensor_count_(sensor_count) {}

      void UpdateAt(std::size_t index, std::uint16_t raw_value,
                    std::uint32_t timestamp_ticks) noexcept {
        if (sensors_ == nullptr || index >= sensor_count_) {
          return;
        }
        domain::sensors::SensorState& state = sensors_[index];
        state.last_raw_value = raw_value;
        state.last_processed_value = static_cast<float>(raw_value);
        state.last_timestamp_ticks = timestamp_ticks;
      }

     private:
      domain::sensors::SensorState* sensors_ = nullptr;
      std::size_t sensor_count_ = 0;
    };

    domain::sensors::SensorState s[22] = {
        domain::sensors::SensorState{1},  domain::sensors::SensorState{2},
        domain::sensors::SensorState{3},  domain::sensors::SensorState{4},
        domain::sensors::SensorState{5},  domain::sensors::SensorState{6},
        domain::sensors::SensorState{7},  domain::sensors::SensorState{8},
        domain::sensors::SensorState{9},  domain::sensors::SensorState{10},
        domain::sensors::SensorState{11}, domain::sensors::SensorState{12},
        domain::sensors::SensorState{13}, domain::sensors::SensorState{14},
        domain::sensors::SensorState{15}, domain::sensors::SensorState{16},
        domain::sensors::SensorState{17}, domain::sensors::SensorState{18},
        domain::sensors::SensorState{19}, domain::sensors::SensorState{20},
        domain::sensors::SensorState{21}, domain::sensors::SensorState{22},
    };
    TestSensorGroup group(s, 22);

    SECTION("When called with one ADC1 sequence") {
      SECTION("Should map ranks to the correct sensor IDs and update timestamps") {
        const std::uint16_t values[7] = {101, 103, 105, 107, 109, 111, 112};

        app::analog::AdcRankMappedFrameDecoder decoder;
        decoder.ApplySequence(values, 7, app::config_sensors::kAdc1SensorIdByRank, group, 123u);

        REQUIRE(s[0].last_raw_value == 101);
        REQUIRE(s[2].last_raw_value == 103);
        REQUIRE(s[4].last_raw_value == 105);
        REQUIRE(s[6].last_raw_value == 107);
        REQUIRE(s[8].last_raw_value == 109);
        REQUIRE(s[10].last_raw_value == 111);
        REQUIRE(s[11].last_raw_value == 112);
        REQUIRE(s[11].last_timestamp_ticks == 123u);
      }
    }

    SECTION("When called with one ADC3 sequence") {
      SECTION("Should map ranks to the correct sensor IDs and update timestamps") {
        const std::uint16_t values[8] = {213, 214, 217, 218, 219, 220, 221, 222};

        app::analog::AdcRankMappedFrameDecoder decoder;
        decoder.ApplySequence(values, 8, app::config_sensors::kAdc3SensorIdByRank, group, 987u);

        REQUIRE(s[12].last_raw_value == 213);
        REQUIRE(s[21].last_raw_value == 222);
        REQUIRE(s[21].last_timestamp_ticks == 987u);
      }
    }
  }
}

#endif
