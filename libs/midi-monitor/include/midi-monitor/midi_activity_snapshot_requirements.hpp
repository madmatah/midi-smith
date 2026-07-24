#pragma once

#include "midi-monitor/midi_activity_snapshot.hpp"

namespace midismith::midi_monitor {

class MidiActivitySnapshotRequirements {
 public:
  virtual ~MidiActivitySnapshotRequirements() = default;

  virtual MidiActivitySnapshot CaptureSnapshot() const noexcept = 0;
};

}  // namespace midismith::midi_monitor
