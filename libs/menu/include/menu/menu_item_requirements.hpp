#pragma once

#include <string_view>

namespace midismith::menu {

class MenuControllerRequirements;

inline constexpr char kNoTrailingGlyph = '\0';

class MenuItemRequirements {
 public:
  virtual ~MenuItemRequirements() = default;

  virtual std::string_view label() const noexcept = 0;
  virtual char trailing_glyph() const noexcept {
    return kNoTrailingGlyph;
  }
  virtual void Activate(MenuControllerRequirements& controller) noexcept = 0;
};

}  // namespace midismith::menu
