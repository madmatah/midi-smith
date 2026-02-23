#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>

#include "sensor-linearization/sensor_linear_processor_configuration.hpp"

namespace midismith::sensor_linearization {

template <std::size_t kLookupTableSize>
class SensorLinearProcessor {
  static_assert(kLookupTableSize >= 2u, "kLookupTableSize must be >= 2");

 public:
  using Configuration = SensorLinearProcessorConfiguration<kLookupTableSize>;

  void Reset() noexcept {}

  void ApplyConfiguration(const Configuration* configuration) noexcept {
    configuration_.store(configuration, std::memory_order_release);
  }

  const Configuration* GetConfiguration() const noexcept {
    return configuration_.load(std::memory_order_acquire);
  }

  template <typename ContextT>
  float Transform(float input, const ContextT& ctx) const noexcept {
    (void) ctx;

    const Configuration* configuration = configuration_.load(std::memory_order_acquire);
    if (configuration == nullptr || configuration->lookup_table == nullptr) {
      return 1.0f;
    }

    float t =
        input * configuration->input_to_lut_index_scale + configuration->input_to_lut_index_bias;
    t = std::clamp(t, 0.0f, static_cast<float>(kLookupTableSize - 1u));

    const float idx_f = std::floor(t);
    const std::size_t idx = static_cast<std::size_t>(idx_f);
    if (idx + 1u >= kLookupTableSize) {
      return (*configuration->lookup_table)[kLookupTableSize - 1u];
    }

    const float frac = t - idx_f;
    const float a = (*configuration->lookup_table)[idx];
    const float b = (*configuration->lookup_table)[idx + 1u];
    return a + frac * (b - a);
  }

 private:
  std::atomic<const Configuration*> configuration_{nullptr};
};

}  // namespace midismith::sensor_linearization
