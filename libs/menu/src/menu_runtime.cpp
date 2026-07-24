#include "menu/menu_runtime.hpp"

#include "menu/input_event.hpp"
#include "menu/menu_screen_requirements.hpp"
#include "text-display/text_display_requirements.hpp"

namespace midismith::menu {

MenuRuntime::MenuRuntime(MenuScreenRequirements& root_screen,
                         MenuScreenRequirements** stack_storage,
                         std::size_t stack_capacity) noexcept
    : stack_(stack_storage, stack_capacity) {
  Push(root_screen);
}

void MenuRuntime::HandleInput(InputEvent event) noexcept {
  MenuScreenRequirements* current_screen = stack_.top();
  if (current_screen == nullptr) {
    return;
  }
  const bool consumed = current_screen->HandleInput(event, *this);
  if (!consumed && event.kind == InputEvent::Kind::kButtonLongPress) {
    Pop();
  }
  dirty_ = dirty_ || current_screen->is_dirty();
}

void MenuRuntime::Render(midismith::text_display::TextDisplayRequirements& display) noexcept {
  MenuScreenRequirements* current_screen = stack_.top();
  if (current_screen == nullptr) {
    return;
  }
  if (dirty_ || current_screen->is_dirty()) {
    current_screen->Render(display);
    dirty_ = false;
  }
}

bool MenuRuntime::is_dirty() const noexcept {
  const MenuScreenRequirements* current_screen = stack_.top();
  return dirty_ || (current_screen != nullptr && current_screen->is_dirty());
}

bool MenuRuntime::Push(MenuScreenRequirements& screen) noexcept {
  if (!stack_.Push(screen)) {
    return false;
  }
  screen.OnEnter(*this);
  dirty_ = true;
  return true;
}

std::string_view MenuRuntime::parent_title() const noexcept {
  const MenuScreenRequirements* parent_screen = stack_.below_top();
  return parent_screen == nullptr ? std::string_view{} : parent_screen->title();
}

bool MenuRuntime::Pop() noexcept {
  if (stack_.size() <= 1) {
    return false;
  }
  if (!stack_.Pop()) {
    return false;
  }
  MenuScreenRequirements* current_screen = stack_.top();
  if (current_screen != nullptr) {
    current_screen->OnEnter(*this);
  }
  dirty_ = true;
  return true;
}

}  // namespace midismith::menu
