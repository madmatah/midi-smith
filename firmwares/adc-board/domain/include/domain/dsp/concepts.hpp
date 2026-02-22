#pragma once

#include <concepts>

namespace midismith::adc_board::domain::dsp::concepts {

template <typename T>
concept Resettable = requires(T t) {
  { t.Reset() } -> std::same_as<void>;
};

template <typename T, typename ContextT>
concept SignalTransformer = requires(T t, float input, const ContextT& ctx) {
  { t.Transform(input, ctx) } -> std::same_as<float>;
};

template <typename T, typename ContextT>
concept DecimatableSignalTransformer = requires(T t, float input, const ContextT& ctx) {
  { t.Push(input, ctx) } -> std::same_as<void>;
  { t.Compute(input, ctx) } -> std::same_as<float>;
};

}  // namespace midismith::adc_board::domain::dsp::concepts
