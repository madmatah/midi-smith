#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "domain/music/piano/detail/null_key_action_handler.hpp"
#include "domain/music/piano/key_action_requirements.hpp"
#include "domain/music/piano/velocity/constant_velocity_mapper.hpp"
#include "domain/music/piano/velocity/velocity_mapper_requirements.hpp"

namespace midismith::adc_board::domain::music::piano {

template <typename MapperT, float kActiveZone, float kLetoff, float kStrike, float kDrop,
          float kRearm>
class MidiVelocityEngine {
 public:
  static_assert(std::is_base_of_v<velocity::VelocityMapperRequirements, MapperT>,
                "MapperT must derive from VelocityMapperRequirements.");
  static_assert(std::is_default_constructible_v<MapperT>, "MapperT must be default-constructible.");

  void Reset() noexcept {
    state_ = State::IDLE;
    has_prev_position_ = false;
    prev_position_ = 1.0f;
    latched_velocity_ = 0u;
  }

  void SetKeyActionHandler(KeyActionRequirements* handler) noexcept {
    handler_ = (handler != nullptr) ? handler : &null_handler_;
  }

  template <typename ContextT>
  void Execute(float position, ContextT& ctx) noexcept {
    if (!has_prev_position_) {
      has_prev_position_ = true;
      prev_position_ = position;
      return;
    }

    const float speed_m_per_s = ctx.sensor.last_hammer_speed_m_per_s;

    switch (state_) {
      case State::IDLE:
        if (position < kActiveZone) {
          state_ = State::TRACKING;
          latched_velocity_ = 0u;
        }
        break;

      case State::TRACKING:
        if (position >= kActiveZone) {
          state_ = State::IDLE;
          latched_velocity_ = 0u;
          break;
        }

        if (prev_position_ > kLetoff && position <= kLetoff && speed_m_per_s < 0.0f) {
          const float latched_speed = -speed_m_per_s;
          latched_velocity_ = mapper_impl_.Map(latched_speed);
          state_ = State::LATCHED;
        }
        break;

      case State::LATCHED:
        if (position <= kStrike) {
          handler_->OnNoteOn(latched_velocity_);
          ctx.sensor.last_midi_velocity = latched_velocity_;
          ctx.sensor.is_note_on = true;
          state_ = State::SUSTAINED;
          break;
        }

        if (speed_m_per_s > 0.0f && position >= kDrop) {
          latched_velocity_ = 0u;
          state_ = State::IDLE;
        }
        break;

      case State::SUSTAINED:
        if (position >= kRearm) {
          latched_velocity_ = 0u;
          state_ = State::TRACKING;
        }
        break;
    }

    prev_position_ = position;
  }

 private:
  enum class State : std::uint8_t {
    IDLE = 0,
    TRACKING,
    LATCHED,
    SUSTAINED,
  };

  State state_ = State::IDLE;
  bool has_prev_position_ = false;
  float prev_position_ = 1.0f;
  midismith::midi::Velocity latched_velocity_ = 0u;

  MapperT mapper_impl_{};
  detail::NullKeyActionHandler null_handler_{};

  KeyActionRequirements* handler_ = &null_handler_;
};

template <float kActiveZone, float kLetoff, float kStrike, float kDrop, float kRearm>
using DefaultMidiVelocityEngine = MidiVelocityEngine<velocity::ConstantVelocityMapper<64u>,
                                                     kActiveZone, kLetoff, kStrike, kDrop, kRearm>;

}  // namespace midismith::adc_board::domain::music::piano
