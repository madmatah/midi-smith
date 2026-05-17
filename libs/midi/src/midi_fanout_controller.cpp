#include "midi/midi_fanout_controller.hpp"

namespace midismith::midi {

MidiFanoutController::MidiFanoutController(MidiControllerRequirements* const* sinks,
                                           std::size_t sink_count) noexcept
    : sinks_(sinks), sink_count_(sink_count) {}

void MidiFanoutController::SendRawMessage(const uint8_t* data, uint8_t length) noexcept {
  if (sinks_ == nullptr) {
    return;
  }
  for (std::size_t i = 0; i < sink_count_; ++i) {
    if (sinks_[i] != nullptr) {
      sinks_[i]->SendRawMessage(data, length);
    }
  }
}

}  // namespace midismith::midi
