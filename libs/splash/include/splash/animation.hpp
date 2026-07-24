#pragma once

#include "splash/pixel_canvas.hpp"

namespace midismith::splash {

inline constexpr int kDisplayWidth = 160;
inline constexpr int kDisplayHeight = 80;
inline constexpr double kAnimationDurationSeconds = 3.55;

void RenderFrame(double time_seconds, PixelCanvas& canvas) noexcept;

}  // namespace midismith::splash
