#include "menu/title_bar.hpp"

#include "menu/text_layout.hpp"
#include "text-display/glyphs.hpp"

namespace midismith::menu {

void RenderTitleBar(midismith::text_display::TextDisplayRequirements& display,
                    std::string_view parent_title, std::string_view title) noexcept {
  namespace glyphs = midismith::text_display::glyphs;
  display.FillRow(0, midismith::text_display::CellAttribute::kTitle);
  const std::size_t breadcrumb_length = parent_title.size() + 1 + title.size();
  if (parent_title.empty() || breadcrumb_length > display.columns()) {
    display.DrawText(0, CenteredColumn(display.columns(), title.size()), title,
                     midismith::text_display::CellAttribute::kTitle);
    return;
  }
  const std::uint8_t start_column = CenteredColumn(display.columns(), breadcrumb_length);
  display.DrawText(0, start_column, parent_title, midismith::text_display::CellAttribute::kTitle);
  display.DrawText(0, static_cast<std::uint8_t>(start_column + parent_title.size()),
                   std::string_view(&glyphs::kChevronRight, 1),
                   midismith::text_display::CellAttribute::kTitle);
  display.DrawText(0, static_cast<std::uint8_t>(start_column + parent_title.size() + 1), title,
                   midismith::text_display::CellAttribute::kTitle);
}

}  // namespace midismith::menu
