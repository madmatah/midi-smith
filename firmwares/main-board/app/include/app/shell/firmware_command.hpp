#pragma once

#include <string_view>

#include "app/shell/removable_storage_requirements.hpp"
#include "app/shell/self_update_requirements.hpp"
#include "bsp-types/board_reset_requirements.hpp"
#include "product-id/product_id.hpp"
#include "shell/command_requirements.hpp"
#include "update-catalogue/image_source_requirements.hpp"

namespace midismith::main_board::app::shell {

class FirmwareCommand final : public midismith::shell::CommandRequirements {
 public:
  FirmwareCommand(RemovableStorageRequirements& storage,
                  midismith::update_catalogue::ImageSourceRequirements& images,
                  SelfUpdateRequirements& self_update,
                  midismith::bsp::BoardResetRequirements& board_reset,
                  std::string_view installed_version) noexcept;

  std::string_view Name() const noexcept override {
    return "firmware";
  }
  std::string_view Help() const noexcept override {
    return "Report and update board firmware (firmware <status|update self>)";
  }
  void Run(int argc, char** argv, midismith::io::WritableStreamRequirements& out) noexcept override;

 private:
  void ReportStatus(midismith::io::WritableStreamRequirements& out) noexcept;
  void ReportBoardLine(midismith::product_id::ProductId product,
                       midismith::io::WritableStreamRequirements& out) noexcept;
  void UpdateSelf(midismith::io::WritableStreamRequirements& out) noexcept;

  RemovableStorageRequirements& storage_;
  midismith::update_catalogue::ImageSourceRequirements& images_;
  SelfUpdateRequirements& self_update_;
  midismith::bsp::BoardResetRequirements& board_reset_;
  std::string_view installed_version_;
};

}  // namespace midismith::main_board::app::shell
