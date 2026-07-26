#include "firmware-staging/staging_writer.hpp"

#include <algorithm>

namespace midismith::firmware_staging {

namespace {

constexpr std::uint8_t kErasedFlashByte = 0xFF;

}  // namespace

StagingOutcome StagingWriter::Begin(std::size_t container_size_bytes) noexcept {
  if (container_size_bytes == 0 || container_size_bytes > slot_.CapacityBytes()) {
    return StagingOutcome::kContainerDoesNotFitTheSlot;
  }

  if (!slot_.Erase()) {
    return StagingOutcome::kEraseFailed;
  }

  announced_size_bytes_ = container_size_bytes;
  accepted_bytes_ = 0;
  programmed_bytes_ = 0;
  pending_bytes_ = 0;
  pending_word_.fill(kErasedFlashByte);
  started_ = true;
  return StagingOutcome::kStaged;
}

StagingOutcome StagingWriter::Write(std::span<const std::uint8_t> chunk) noexcept {
  if (!started_) {
    return StagingOutcome::kWriterNotStarted;
  }
  if (accepted_bytes_ + chunk.size() > announced_size_bytes_) {
    return StagingOutcome::kMoreBytesThanAnnounced;
  }

  while (!chunk.empty()) {
    const std::size_t room = pending_word_.size() - pending_bytes_;
    const std::size_t taken = std::min(room, chunk.size());
    std::copy_n(chunk.begin(), taken, pending_word_.begin() + pending_bytes_);
    pending_bytes_ += taken;
    accepted_bytes_ += taken;
    chunk = chunk.subspan(taken);

    if (pending_bytes_ == pending_word_.size()) {
      if (!slot_.ProgramFlashWord(programmed_bytes_, pending_word_)) {
        started_ = false;
        return StagingOutcome::kProgramFailed;
      }
      programmed_bytes_ += pending_word_.size();
      pending_bytes_ = 0;
      pending_word_.fill(kErasedFlashByte);
    }
  }

  return StagingOutcome::kStaged;
}

StagingOutcome StagingWriter::Finish(
    const firmware_image::TargetConstraints& constraints) noexcept {
  if (!started_) {
    return StagingOutcome::kWriterNotStarted;
  }
  if (accepted_bytes_ < announced_size_bytes_) {
    return StagingOutcome::kFewerBytesThanAnnounced;
  }

  if (pending_bytes_ != 0) {
    if (!slot_.ProgramFlashWord(programmed_bytes_, pending_word_)) {
      started_ = false;
      return StagingOutcome::kProgramFailed;
    }
    programmed_bytes_ += pending_word_.size();
    pending_bytes_ = 0;
  }

  const auto staged = slot_.Contents().first(announced_size_bytes_);
  const auto parsed = firmware_image::ParseImageHeader(staged);
  if (!parsed.is_valid()) {
    return StagingOutcome::kStagedContainerRejected;
  }

  const auto payload = firmware_image::ContainerPayload(parsed.header, staged);
  if (!payload.has_value()) {
    return StagingOutcome::kStagedContainerRejected;
  }

  if (firmware_image::EvaluateImageInstallability(parsed.header, *payload, constraints) !=
      firmware_image::ImageInstallability::kInstallable) {
    return StagingOutcome::kStagedContainerRejected;
  }

  return StagingOutcome::kStaged;
}

}  // namespace midismith::firmware_staging
