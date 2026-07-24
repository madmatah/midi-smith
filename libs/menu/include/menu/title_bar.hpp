#pragma once

#include <string_view>

#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

void RenderTitleBar(midismith::text_display::TextDisplayRequirements& display,
                    std::string_view parent_title, std::string_view title) noexcept;

}  // namespace midismith::menu
