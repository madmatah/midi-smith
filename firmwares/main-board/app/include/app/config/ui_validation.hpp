#pragma once

#include "app/config/ui.hpp"
#include "splash/animation.hpp"

namespace midismith::main_board::app::config {

static_assert(kTftTextColumns * kTftFontWidth == midismith::splash::kDisplayWidth);
static_assert(kTftTextRows * kTftFontHeight == midismith::splash::kDisplayHeight);
static_assert(midismith::splash::kDisplayHeight % kSplashBandRows == 0);
static_assert(kSplashFramePeriodMs > 0);

}  // namespace midismith::main_board::app::config
