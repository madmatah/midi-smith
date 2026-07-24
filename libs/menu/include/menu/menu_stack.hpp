#pragma once

#include <cstddef>

namespace midismith::menu {

class MenuScreenRequirements;

class MenuStack {
 public:
  MenuStack(MenuScreenRequirements** storage, std::size_t capacity) noexcept;

  bool Push(MenuScreenRequirements& screen) noexcept;
  bool Pop() noexcept;

  MenuScreenRequirements* top() noexcept;
  const MenuScreenRequirements* top() const noexcept;
  const MenuScreenRequirements* below_top() const noexcept;
  std::size_t size() const noexcept;
  std::size_t capacity() const noexcept;
  bool is_empty() const noexcept;

 private:
  MenuScreenRequirements** storage_;
  std::size_t capacity_;
  std::size_t size_ = 0;
};

}  // namespace midismith::menu
