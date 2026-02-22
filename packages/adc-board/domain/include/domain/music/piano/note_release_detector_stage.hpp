#pragma once

#include <type_traits>

#include "domain/music/piano/detail/null_key_action_handler.hpp"
#include "domain/music/piano/key_action_requirements.hpp"
#include "domain/music/piano/velocity/constant_velocity_mapper.hpp"
#include "domain/music/piano/velocity/velocity_mapper_requirements.hpp"

namespace midismith::adc_board::domain::music::piano {

template <typename MapperT, float kReleaseThreshold>
class NoteReleaseDetectorStage {
 public:
  static_assert(std::is_base_of_v<velocity::VelocityMapperRequirements, MapperT>,
                "MapperT must derive from VelocityMapperRequirements.");
  static_assert(std::is_default_constructible_v<MapperT>, "MapperT must be default-constructible.");

  void Reset() noexcept {
    has_prev_position_ = false;
    prev_position_ = 1.0f;
    prev_is_note_on_ = false;
    latched_release_speed_m_per_s_ = 0.0f;
  }

  void SetKeyActionHandler(KeyActionRequirements* handler) noexcept {
    handler_ = (handler != nullptr) ? handler : &null_handler_;
  }

  template <typename ContextT>
  void Execute(float last_shank_position_norm, ContextT& ctx) noexcept {
    const bool is_note_on = ctx.sensor.is_note_on;
    const float position = ctx.sensor.last_shank_position_smoothed_norm;

    if (!has_prev_position_) {
      has_prev_position_ = true;
      prev_position_ = position;
      prev_is_note_on_ = is_note_on;
      latched_release_speed_m_per_s_ = 0.0f;

      if (is_note_on && position < kReleaseThreshold) {
        const float speed_m_per_s = ctx.sensor.last_shank_falling_speed_m_per_s;
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
      const float speed_m_per_s = ctx.sensor.last_shank_falling_speed_m_per_s;
      if (speed_m_per_s > 0.0f) {
        latched_release_speed_m_per_s_ = speed_m_per_s;
      }
    }

    if (is_note_on && prev_position_ < kReleaseThreshold && position >= kReleaseThreshold) {
      const midismith::common::domain::music::Velocity release_velocity =
          mapper_impl_.Map(latched_release_speed_m_per_s_);
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

  MapperT mapper_impl_{};
  detail::NullKeyActionHandler null_handler_{};

  KeyActionRequirements* handler_ = &null_handler_;
};

template <float kReleaseThreshold>
using DefaultNoteReleaseDetectorStage =
    NoteReleaseDetectorStage<velocity::ConstantVelocityMapper<127u>, kReleaseThreshold>;

}  // namespace midismith::adc_board::domain::music::piano
