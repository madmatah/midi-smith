#include "menu/menu_stack.hpp"

#include "menu/menu_screen_requirements.hpp"

namespace midismith::menu {

MenuStack::MenuStack(MenuScreenRequirements** storage, std::size_t capacity) noexcept
    : storage_(storage), capacity_(capacity) {}

bool MenuStack::Push(MenuScreenRequirements& screen) noexcept {
  if (size_ >= capacity_) {
    return false;
  }
  storage_[size_] = &screen;
  size_++;
  return true;
}

bool MenuStack::Pop() noexcept {
  if (size_ == 0) {
    return false;
  }
  size_--;
  storage_[size_] = nullptr;
  return true;
}

MenuScreenRequirements* MenuStack::top() noexcept {
  if (size_ == 0) {
    return nullptr;
  }
  return storage_[size_ - 1];
}

const MenuScreenRequirements* MenuStack::top() const noexcept {
  if (size_ == 0) {
    return nullptr;
  }
  return storage_[size_ - 1];
}

const MenuScreenRequirements* MenuStack::below_top() const noexcept {
  if (size_ < 2) {
    return nullptr;
  }
  return storage_[size_ - 2];
}

std::size_t MenuStack::size() const noexcept {
  return size_;
}

std::size_t MenuStack::capacity() const noexcept {
  return capacity_;
}

bool MenuStack::is_empty() const noexcept {
  return size_ == 0;
}

}  // namespace midismith::menu
