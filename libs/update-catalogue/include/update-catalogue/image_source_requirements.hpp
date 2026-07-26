#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>

namespace midismith::update_catalogue {

inline constexpr std::size_t kMaxImagePathLengthBytes = 63;

class ImageSourceRequirements {
 public:
  virtual ~ImageSourceRequirements() = default;

  [[nodiscard]] virtual std::optional<std::uint32_t> SizeOf(std::string_view path) noexcept = 0;

  [[nodiscard]] virtual std::optional<std::size_t> ReadAt(std::string_view path,
                                                          std::uint32_t offset_bytes,
                                                          std::span<std::uint8_t> out) noexcept = 0;
};

}  // namespace midismith::update_catalogue
