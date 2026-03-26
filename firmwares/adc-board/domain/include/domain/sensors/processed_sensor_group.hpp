#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "domain/sensors/sensor_state.hpp"
#include "dsp/concepts.hpp"

namespace midismith::adc_board::domain::sensors {

template <typename ProcessorT, typename ContextT>
class ProcessedSensorGroup {
 public:
  static_assert(midismith::dsp::concepts::SignalTransformer<ProcessorT, ContextT>,
                "ProcessorT must satisfy SignalTransformer for ContextT");
  static_assert(std::is_constructible_v<ContextT, std::uint32_t, SensorState&>,
                "ContextT must be constructible from (timestamp_ticks, sensor_state)");

  using Context = ContextT;

  ProcessedSensorGroup(SensorState* sensors, ProcessorT* processors,
                       std::size_t sensor_count) noexcept
      : sensors_(sensors), processors_(processors), sensor_count_(sensor_count) {
    assert(sensors != nullptr);
    assert(processors != nullptr);
  }

  std::size_t count() const noexcept {
    return sensor_count_;
  }

  template <typename Fn>
  void ForEachEntry(Fn fn) noexcept {
    for (std::size_t i = 0; i < sensor_count_; ++i) {
      fn(sensors_[i], processors_[i]);
    }
  }

  void UpdateAt(std::size_t index, std::uint16_t raw_value,
                std::uint32_t timestamp_ticks) noexcept {
    assert(index < sensor_count_);
    SensorState& state = sensors_[index];

    state.last_raw_value = raw_value;
    state.last_timestamp_ticks = timestamp_ticks;

    ProcessorT& processor = processors_[index];
    const float raw_float = static_cast<float>(raw_value);
    ContextT ctx{timestamp_ticks, state};

    const float processed_value = processor.Transform(raw_float, ctx);

    state.last_processed_value = processed_value;
  }

 private:
  SensorState* sensors_ = nullptr;
  ProcessorT* processors_ = nullptr;
  std::size_t sensor_count_ = 0;
};

}  // namespace midismith::adc_board::domain::sensors
