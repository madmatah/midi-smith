namespace domain::dsp::logic {

template <float kThreshold, auto ControlValueProvider>
class InputGate {
 public:
  template <typename ContextT>
  float Transform(float sample, ContextT& ctx) noexcept {
    if (ControlValueProvider(ctx) >= kThreshold) {
      return 0.0f;
    }
    return sample;
  }
  void Reset() noexcept {}
};

}  // namespace domain::dsp::logic
