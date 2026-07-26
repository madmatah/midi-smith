#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

#include "app/shell/removable_storage_requirements.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::main_board::bsp::storage {

class SdCardImageSource final
    : public midismith::update_catalogue::ImageSourceRequirements,
      public midismith::main_board::app::shell::RemovableStorageRequirements {
 public:
  [[nodiscard]] bool Mount() noexcept override;

  [[nodiscard]] midismith::bsp::storage::SdCardBringUpOutcome last_bring_up_outcome()
      const noexcept override;

  void Unmount() noexcept override;

  [[nodiscard]] bool is_mounted() const noexcept {
    return mounted_;
  }

  [[nodiscard]] std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept override;

  [[nodiscard]] std::optional<std::size_t> ReadAt(std::string_view path, std::uint32_t offset_bytes,
                                                  std::span<std::uint8_t> out) noexcept override;

 private:
  bool mounted_ = false;
};

}  // namespace midismith::main_board::bsp::storage
