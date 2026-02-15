#pragma once

#include "domain/music/piano/detail/null_key_action_handler.hpp"
#include "domain/music/piano/goebl_logarithmic_velocity_mapper.hpp"
#include "domain/music/piano/key_action_requirements.hpp"
#include "domain/music/piano/velocity_mapper_requirements.hpp"

namespace domain::music::piano {

template <float kReleaseThreshold>
class NoteReleaseDetectorStage {
 public:
  void Reset() noexcept {
    has_prev_position_ = false;
    prev_position_ = 1.0f;
    prev_is_note_on_ = false;
    latched_release_speed_m_per_s_ = 0.0f;
  }

  void SetVelocityMapper(VelocityMapperRequirements* mapper) noexcept {
    mapper_ = (mapper != nullptr) ? mapper : &mapper_impl_;
  }

  void SetKeyActionHandler(KeyActionRequirements* handler) noexcept {
    handler_ = (handler != nullptr) ? handler : &null_handler_;
  }

  template <typename ContextT>
  void Execute(float position, ContextT& ctx) noexcept {
    const bool is_note_on = ctx.sensor.is_note_on;

    if (!has_prev_position_) {
      has_prev_position_ = true;
      prev_position_ = position;
      prev_is_note_on_ = is_note_on;
      latched_release_speed_m_per_s_ = 0.0f;

      if (is_note_on && position < kReleaseThreshold) {
        const float speed_m_per_s = ctx.sensor.last_speed_m_per_s;
        if (speed_m_per_s > 0.0f) {
          latched_release_speed_m_per_s_ = speed_m_per_s;
        }
      }
    }

    if (!prev_is_note_on_ && is_note_on) {
      latched_release_speed_m_per_s_ = 0.0f;
    }

    if (!is_note_on) {
      latched_release_speed_m_per_s_ = 0.0f;
    }

    if (is_note_on && position < kReleaseThreshold) {
      const float speed_m_per_s = ctx.sensor.last_speed_m_per_s;
      if (speed_m_per_s > 0.0f) {
        latched_release_speed_m_per_s_ = speed_m_per_s;
      }
    }

    if (is_note_on && prev_position_ < kReleaseThreshold && position >= kReleaseThreshold) {
      const domain::music::Velocity release_velocity = mapper_->Map(latched_release_speed_m_per_s_);
      handler_->OnNoteOff(release_velocity);
      ctx.sensor.is_note_on = false;
      latched_release_speed_m_per_s_ = 0.0f;
    }

    prev_position_ = position;
    prev_is_note_on_ = ctx.sensor.is_note_on;
  }

 private:
  bool has_prev_position_ = false;
  float prev_position_ = 1.0f;
  bool prev_is_note_on_ = false;
  float latched_release_speed_m_per_s_ = 0.0f;

  GoeblLogarithmicVelocityMapper mapper_impl_{};
  detail::NullKeyActionHandler null_handler_{};

  VelocityMapperRequirements* mapper_ = &mapper_impl_;
  KeyActionRequirements* handler_ = &null_handler_;
};

}  // namespace domain::music::piano
