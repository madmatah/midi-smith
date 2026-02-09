#pragma once

#include <concepts>

namespace domain::dsp::concepts {

template <typename T>
concept Resettable = requires(T t) {
  { t.Reset() } -> std::same_as<void>;
};

template <typename T, typename ContextT>
concept SignalTransformer = requires(T t, float input, const ContextT& ctx) {
  { t.Transform(input, ctx) } -> std::same_as<float>;
};

template <typename T, typename ContextT>
concept SignalConsumer = requires(T t, float input, ContextT& ctx) {
  { t.Execute(input, ctx) } -> std::same_as<void>;
};

template <typename T, typename ContextT>
concept DecimatableSignalTransformer = requires(T t, float input, const ContextT& ctx) {
  { t.Push(input, ctx) } -> std::same_as<void>;
  { t.Compute(input, ctx) } -> std::same_as<float>;
};

}  // namespace domain::dsp::concepts
