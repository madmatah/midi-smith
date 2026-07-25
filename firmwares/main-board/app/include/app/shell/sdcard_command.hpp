#pragma once

#include <string_view>

#include "app/shell/removable_storage_requirements.hpp"
#include "product-id/product_id.hpp"
#include "shell/command_requirements.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::main_board::app::shell {

class SdCardCommand final : public midismith::shell::CommandRequirements {
 public:
  SdCardCommand(RemovableStorageRequirements& storage,
                midismith::update_catalogue::ImageSourceRequirements& images,
                std::string_view installed_version) noexcept;

  std::string_view Name() const noexcept override {
    return "sdcard";
  }
  std::string_view Help() const noexcept override {
    return "Mount the SD card and report the firmware images it carries";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  void ReportImageFor(midismith::product_id::ProductId product,
                      midismith::io::WritableStreamRequirements& out) noexcept;

  RemovableStorageRequirements& storage_;
  midismith::update_catalogue::ImageSourceRequirements& images_;
  std::string_view installed_version_;
};

}  // namespace midismith::main_board::app::shell
