#pragma once

#include <cstddef>
#include <cstdint>

#include "app/analog/adc_dma_control_requirements.hpp"
#include "os/queue.hpp"

namespace bsp::adc {

enum class AdcGroup : std::uint8_t {
  kAdc1 = 0,
  kAdc2 = 1,
  kAdc3 = 2,
};

struct AdcFrameDescriptor {
  AdcGroup group;
  std::uint8_t half;
  std::uint32_t sequence_id;
  std::uint32_t timestamp_ticks;
  const void* data;
  std::uint16_t element_count;
  std::uint8_t element_size_bytes;
};

class AdcDma final : public app::analog::AdcDmaControlRequirements {
 public:
  static constexpr std::size_t kAdc1RanksPerSequence = 7;
  static constexpr std::size_t kAdc2RanksPerSequence = 7;
  static constexpr std::size_t kAdc3RanksPerSequence = 8;

  static constexpr std::size_t kMaxSequencesPerHalfBuffer = 32;
  static constexpr std::size_t kMaxAdc1HalfwordsPerHalfBuffer =
      kMaxSequencesPerHalfBuffer * kAdc1RanksPerSequence;
  static constexpr std::size_t kMaxAdc2HalfwordsPerHalfBuffer =
      kMaxSequencesPerHalfBuffer * kAdc2RanksPerSequence;
  static constexpr std::size_t kMaxAdc3HalfwordsPerHalfBuffer =
      kMaxSequencesPerHalfBuffer * kAdc3RanksPerSequence;

  static constexpr std::size_t kMaxAdc1HalfwordsPerBuffer = 2 * kMaxAdc1HalfwordsPerHalfBuffer;
  static constexpr std::size_t kMaxAdc2HalfwordsPerBuffer = 2 * kMaxAdc2HalfwordsPerHalfBuffer;
  static constexpr std::size_t kMaxAdc3HalfwordsPerBuffer = 2 * kMaxAdc3HalfwordsPerHalfBuffer;

  explicit AdcDma(os::Queue<AdcFrameDescriptor, 8>& queue) noexcept;

  bool Start() noexcept override;
  void Stop() noexcept override;

  void HandleHalfComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept;
  void HandleFullComplete(AdcGroup group, std::uint32_t timestamp_ticks) noexcept;

 private:
  std::uint16_t adc1_halfwords_per_half_buffer_ = 0;
  std::uint16_t adc2_halfwords_per_half_buffer_ = 0;
  std::uint16_t adc3_halfwords_per_half_buffer_ = 0;

  os::Queue<AdcFrameDescriptor, 8>& queue_;
  std::uint32_t adc1_sequence_id_ = 0;
  std::uint32_t adc2_sequence_id_ = 0;
  std::uint32_t adc3_sequence_id_ = 0;
  bool running_ = false;
};

void RegisterAdcDma(AdcDma& adc_dma) noexcept;
AdcDma* GetAdcDma() noexcept;

}  // namespace bsp::adc
