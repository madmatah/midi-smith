#pragma once

namespace midismith::splash {

constexpr double Clamp(double value, double minimum, double maximum) {
  if (value < minimum) {
    return minimum;
  }
  if (value > maximum) {
    return maximum;
  }
  return value;
}

constexpr double SmoothStep(double progress) {
  const double clamped = Clamp(progress, 0.0, 1.0);
  return clamped * clamped * (3.0 - 2.0 * clamped);
}

constexpr double EaseInCubic(double progress) {
  const double clamped = Clamp(progress, 0.0, 1.0);
  return clamped * clamped * clamped;
}

constexpr double EaseOutCubic(double progress) {
  const double clamped = Clamp(progress, 0.0, 1.0);
  const double inverse = 1.0 - clamped;
  return 1.0 - inverse * inverse * inverse;
}

constexpr double PhaseProgress(double time_seconds, double start_seconds, double end_seconds) {
  if (end_seconds <= start_seconds) {
    return 1.0;
  }
  return Clamp((time_seconds - start_seconds) / (end_seconds - start_seconds), 0.0, 1.0);
}

}  // namespace midismith::splash
