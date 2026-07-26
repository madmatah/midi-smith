#include "app/shell/firmware_command.hpp"

#include <cstring>

#include "update-catalogue/update_catalogue.hpp"

namespace midismith::main_board::app::shell {

namespace {

using midismith::main_board::app::update::SelfUpdateOutcome;
using midismith::product_id::ProductId;
using midismith::update_catalogue::CatalogueStatus;
using midismith::update_catalogue::EvaluateUpdateNeed;
using midismith::update_catalogue::UpdateCatalogue;
using midismith::update_catalogue::UpdateNeed;

std::string_view NameOf(ProductId product) noexcept {
  switch (product) {
    case ProductId::kMainBoard:
      return "main-board";
    case ProductId::kAdcBoard:
      return "adc-board ";
    case ProductId::kUnknown:
    default:
      return "unknown   ";
  }
}

std::string_view DescribeNeed(UpdateNeed need) noexcept {
  switch (need) {
    case UpdateNeed::kUpToDate:
      return "already running this build";
    case UpdateNeed::kUpdateAvailable:
      return "differs from the running build";
    case UpdateNeed::kInstalledVersionUnknown:
      return "offered, running version unknown until the CAN protocol exists";
    case UpdateNeed::kNoImage:
      return "no image on the card";
    case UpdateNeed::kImageUnusable:
      return "unusable, will not be installed";
  }
  return "unknown";
}

std::string_view DescribeSelfUpdate(SelfUpdateOutcome outcome) noexcept {
  switch (outcome) {
    case SelfUpdateOutcome::kStagedAndPending:
      return "staged, rebooting into the bootloader to install it";
    case SelfUpdateOutcome::kNoImageOnCard:
      return "no main-board image on the card";
    case SelfUpdateOutcome::kImageUnusable:
      return "the image on the card is unusable";
    case SelfUpdateOutcome::kAlreadyRunningThisBuild:
      return "already running this build, nothing to do";
    case SelfUpdateOutcome::kStagingFailed:
      return "the copy into the staging slot failed, nothing was committed";
    case SelfUpdateOutcome::kJournalWriteFailed:
      return "staged, but the pending record could not be written";
  }
  return "unknown outcome";
}

bool Matches(const char* argument, std::string_view expected) noexcept {
  return argument != nullptr && std::string_view{argument} == expected;
}

}  // namespace

FirmwareCommand::FirmwareCommand(RemovableStorageRequirements& storage,
                                 midismith::update_catalogue::ImageSourceRequirements& images,
                                 SelfUpdateRequirements& self_update,
                                 midismith::bsp::BoardResetRequirements& board_reset,
                                 std::string_view installed_version) noexcept
    : storage_(storage),
      images_(images),
      self_update_(self_update),
      board_reset_(board_reset),
      installed_version_(installed_version) {}

void FirmwareCommand::ReportBoardLine(ProductId product,
                                      midismith::io::WritableStreamRequirements& out) noexcept {
  UpdateCatalogue catalogue{images_};
  const auto entry = catalogue.Lookup(product);

  const std::string_view running =
      product == ProductId::kMainBoard ? installed_version_ : std::string_view{};

  out.Write("  ");
  out.Write(NameOf(product));
  out.Write("  running ");
  out.Write(running.empty() ? "unknown" : running);
  out.Write("  offered ");
  out.Write(entry.status == CatalogueStatus::kImageAvailable ? entry.header.version_string.data()
                                                             : "none");
  out.Write("  ");
  out.Write(DescribeNeed(EvaluateUpdateNeed(entry, running)));
  out.Write("\r\n");
}

void FirmwareCommand::ReportStatus(midismith::io::WritableStreamRequirements& out) noexcept {
  if (!storage_.Mount()) {
    out.Write("firmware: the card could not be mounted, no image can be offered\r\n");
    return;
  }

  ReportBoardLine(ProductId::kMainBoard, out);
  ReportBoardLine(ProductId::kAdcBoard, out);
  storage_.Unmount();
}

void FirmwareCommand::UpdateSelf(midismith::io::WritableStreamRequirements& out) noexcept {
  if (!storage_.Mount()) {
    out.Write("firmware: the card could not be mounted\r\n");
    return;
  }

  const SelfUpdateOutcome outcome = self_update_.Run();
  storage_.Unmount();

  out.Write("firmware: ");
  out.Write(DescribeSelfUpdate(outcome));
  out.Write("\r\n");

  if (outcome == SelfUpdateOutcome::kStagedAndPending) {
    out.WaitUntilWritten();
    board_reset_.ResetBoard();
  }
}

void FirmwareCommand::Run(int argc, char** argv,
                          midismith::io::WritableStreamRequirements& out) noexcept {
  if (argc == 2 && Matches(argv[1], "status")) {
    ReportStatus(out);
    return;
  }

  if (argc == 3 && Matches(argv[1], "update") && Matches(argv[2], "self")) {
    UpdateSelf(out);
    return;
  }

  if (argc >= 3 && Matches(argv[1], "update") && Matches(argv[2], "adc")) {
    out.Write("firmware: updating an adc board needs the CAN update protocol, not built yet\r\n");
    return;
  }

  out.Write("usage: firmware <status|update self>\r\n");
}

}  // namespace midismith::main_board::app::shell
