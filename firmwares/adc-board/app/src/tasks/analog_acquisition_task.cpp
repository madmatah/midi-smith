#include "app/tasks/analog_acquisition_task.hpp"

#include <cstddef>
#include <cstdint>

#include "app/analog/acquisition_sequencer.hpp"
#include "app/analog/delay_requirements.hpp"
#include "app/config/config.hpp"
#include "os/queue_requirements.hpp"
#include "os/task.hpp"

namespace midismith::adc_board::app::tasks {
namespace {

inline std::uint32_t DeltaTicks(std::uint32_t now, std::uint32_t then) noexcept {
  return static_cast<std::uint32_t>(now - then);
}

inline std::uint32_t ComputeTicksPerSequence(std::uint32_t now_timestamp_ticks,
                                             std::uint32_t estimate_ticks_per_sequence,
                                             std::uint16_t sequences_per_half_buffer,
                                             bool& has_prev_timestamp,
                                             std::uint32_t& prev_timestamp_ticks) noexcept {
  std::uint32_t ticks_per_sequence = estimate_ticks_per_sequence;
  if (has_prev_timestamp) {
    const std::uint32_t delta = DeltaTicks(now_timestamp_ticks, prev_timestamp_ticks);
    if (sequences_per_half_buffer > 0u) {
      ticks_per_sequence = delta / sequences_per_half_buffer;
    }
  }
  prev_timestamp_ticks = now_timestamp_ticks;
  has_prev_timestamp = true;
  return ticks_per_sequence;
}

template <typename SampleT, std::size_t kRanksPerSequence, typename ApplyFn>
inline void ForEachTimestampedSequenceInHalfBuffer(const SampleT* data,
                                                   std::uint32_t end_timestamp_ticks,
                                                   std::uint32_t ticks_per_sequence,
                                                   std::uint16_t sequences_per_half_buffer,
                                                   ApplyFn apply) noexcept {
  if (data == nullptr || sequences_per_half_buffer == 0u) {
    return;
  }

  const std::uint32_t first_ts = static_cast<std::uint32_t>(
      end_timestamp_ticks -
      static_cast<std::uint32_t>((sequences_per_half_buffer - 1u) * ticks_per_sequence));
  std::uint32_t ts = first_ts;
  const SampleT* seq_ptr = data;
  for (std::size_t seq = 0; seq < sequences_per_half_buffer; ++seq) {
    apply(seq_ptr, ts);
    seq_ptr += kRanksPerSequence;
    ts = static_cast<std::uint32_t>(ts + ticks_per_sequence);
  }
}

class TimestampCounterDelay final : public midismith::adc_board::app::analog::DelayRequirements {
 public:
  explicit TimestampCounterDelay(
      midismith::adc_board::app::time::TimestampCounterRequirements& timestamp_counter) noexcept
      : timestamp_counter_(timestamp_counter) {}

  void DelayUs(std::uint32_t delay_us) noexcept override {
    const std::uint32_t start = timestamp_counter_.NowTicks();
    for (;;) {
      const std::uint32_t now = timestamp_counter_.NowTicks();
      if (DeltaTicks(now, start) >= delay_us) {
        return;
      }
    }
  }

 private:
  midismith::adc_board::app::time::TimestampCounterRequirements& timestamp_counter_;
};

bool ReceiveLatestCommand(
    midismith::os::Queue<midismith::adc_board::app::analog::AcquisitionCommand, 4>& queue,
    midismith::adc_board::app::analog::AcquisitionCommand& cmd) noexcept {
  bool did_receive = false;
  midismith::adc_board::app::analog::AcquisitionCommand tmp{};
  while (queue.Receive(tmp, midismith::os::kNoWait)) {
    cmd = tmp;
    did_receive = true;
  }
  return did_receive;
}

}  // namespace

AnalogAcquisitionTask::AnalogAcquisitionTask(
    midismith::os::Queue<midismith::adc_board::bsp::adc::AdcFrameDescriptor, 8>& queue,
    midismith::os::Queue<midismith::adc_board::app::analog::AcquisitionCommand, 4>& control_queue,
    midismith::common::bsp::GpioRequirements& tia_shutdown,
    midismith::adc_board::bsp::adc::AdcDma& adc_dma,
    midismith::adc_board::app::time::TimestampCounterRequirements& timestamp_counter,
    volatile midismith::adc_board::app::analog::AcquisitionState& state,
    ProcessedSensorGroup& analog_group) noexcept
    : queue_(queue),
      control_queue_(control_queue),
      tia_shutdown_(tia_shutdown),
      adc_dma_(adc_dma),
      timestamp_counter_(timestamp_counter),
      state_(state),
      analog_group_(analog_group) {}

void AnalogAcquisitionTask::entry(void* ctx) noexcept {
  if (ctx == nullptr) {
    return;
  }
  static_cast<AnalogAcquisitionTask*>(ctx)->run();
}

void AnalogAcquisitionTask::ResetDecodingState() noexcept {
  last_adc1_sequence_id_ = 0;
  last_adc2_sequence_id_ = 0;
  last_adc3_sequence_id_ = 0;
  has_prev_adc1_timestamp_ = false;
  has_prev_adc2_timestamp_ = false;
  has_prev_adc3_timestamp_ = false;
  prev_adc1_timestamp_ = 0;
  prev_adc2_timestamp_ = 0;
  prev_adc3_timestamp_ = 0;
}

void AnalogAcquisitionTask::DrainFrameQueue() noexcept {
  for (;;) {
    midismith::adc_board::bsp::adc::AdcFrameDescriptor desc{};
    if (!queue_.Receive(desc, midismith::os::kNoWait)) {
      return;
    }
  }
}

void AnalogAcquisitionTask::EnterDisabledState() noexcept {
  tia_shutdown_.reset();
  adc_dma_.Stop();
  DrainFrameQueue();
  ResetDecodingState();
  state_ = midismith::adc_board::app::analog::AcquisitionState::kDisabled;
}

void AnalogAcquisitionTask::HandleDisabledState(
    midismith::adc_board::app::analog::AcquisitionSequencer& sequencer) noexcept {
  midismith::adc_board::app::analog::AcquisitionCommand cmd{};
  if (!control_queue_.Receive(cmd, midismith::os::kWaitForever)) {
    return;
  }

  if (cmd == midismith::adc_board::app::analog::AcquisitionCommand::kDisable) {
    EnterDisabledState();
    return;
  }

  if (cmd == midismith::adc_board::app::analog::AcquisitionCommand::kEnable) {
    DrainFrameQueue();
    ResetDecodingState();
    const bool started = sequencer.Enable(70);
    if (!started) {
      EnterDisabledState();
      return;
    }
    state_ = midismith::adc_board::app::analog::AcquisitionState::kEnabled;
  }
}

bool AnalogAcquisitionTask::TryHandleDisableRequestWhileEnabled() noexcept {
  midismith::adc_board::app::analog::AcquisitionCommand cmd{};
  if (!ReceiveLatestCommand(control_queue_, cmd)) {
    return false;
  }
  if (cmd != midismith::adc_board::app::analog::AcquisitionCommand::kDisable) {
    return false;
  }
  EnterDisabledState();
  return true;
}

void AnalogAcquisitionTask::ProcessAdc1Frame(
    const midismith::adc_board::bsp::adc::AdcFrameDescriptor& desc) noexcept {
  const std::uint16_t sequences_per_half_buffer = static_cast<std::uint16_t>(
      desc.element_count / midismith::adc_board::bsp::adc::AdcDma::kAdc1RanksPerSequence);
  const std::uint32_t ticks_per_sequence = ComputeTicksPerSequence(
      desc.timestamp_ticks,
      ::midismith::adc_board::app::config::ANALOG_ADC12_TICKS_PER_SEQUENCE_ESTIMATE,
      sequences_per_half_buffer, has_prev_adc1_timestamp_, prev_adc1_timestamp_);

  const auto* values = static_cast<const std::uint16_t*>(desc.data);
  ForEachTimestampedSequenceInHalfBuffer<
      std::uint16_t, midismith::adc_board::bsp::adc::AdcDma::kAdc1RanksPerSequence>(
      values, desc.timestamp_ticks, ticks_per_sequence, sequences_per_half_buffer,
      [this](const std::uint16_t* seq_ptr, std::uint32_t ts) noexcept {
        decoder_.ApplySequence(
            seq_ptr, midismith::adc_board::bsp::adc::AdcDma::kAdc1RanksPerSequence,
            ::midismith::adc_board::app::config::sensors::kAdc1SensorIdByRank, analog_group_, ts);
      });
}

void AnalogAcquisitionTask::ProcessAdc2Frame(
    const midismith::adc_board::bsp::adc::AdcFrameDescriptor& desc) noexcept {
  const std::uint16_t sequences_per_half_buffer = static_cast<std::uint16_t>(
      desc.element_count / midismith::adc_board::bsp::adc::AdcDma::kAdc2RanksPerSequence);
  const std::uint32_t ticks_per_sequence = ComputeTicksPerSequence(
      desc.timestamp_ticks,
      ::midismith::adc_board::app::config::ANALOG_ADC12_TICKS_PER_SEQUENCE_ESTIMATE,
      sequences_per_half_buffer, has_prev_adc2_timestamp_, prev_adc2_timestamp_);

  const auto* values = static_cast<const std::uint16_t*>(desc.data);
  ForEachTimestampedSequenceInHalfBuffer<
      std::uint16_t, midismith::adc_board::bsp::adc::AdcDma::kAdc2RanksPerSequence>(
      values, desc.timestamp_ticks, ticks_per_sequence, sequences_per_half_buffer,
      [this](const std::uint16_t* seq_ptr, std::uint32_t ts) noexcept {
        decoder_.ApplySequence(
            seq_ptr, midismith::adc_board::bsp::adc::AdcDma::kAdc2RanksPerSequence,
            ::midismith::adc_board::app::config::sensors::kAdc2SensorIdByRank, analog_group_, ts);
      });
}

void AnalogAcquisitionTask::ProcessAdc3Frame(
    const midismith::adc_board::bsp::adc::AdcFrameDescriptor& desc) noexcept {
  const std::uint16_t sequences_per_half_buffer = static_cast<std::uint16_t>(
      desc.element_count / midismith::adc_board::bsp::adc::AdcDma::kAdc3RanksPerSequence);
  const std::uint32_t ticks_per_sequence = ComputeTicksPerSequence(
      desc.timestamp_ticks,
      ::midismith::adc_board::app::config::ANALOG_ADC3_TICKS_PER_SEQUENCE_ESTIMATE,
      sequences_per_half_buffer, has_prev_adc3_timestamp_, prev_adc3_timestamp_);

  const auto* values = static_cast<const std::uint16_t*>(desc.data);
  ForEachTimestampedSequenceInHalfBuffer<
      std::uint16_t, midismith::adc_board::bsp::adc::AdcDma::kAdc3RanksPerSequence>(
      values, desc.timestamp_ticks, ticks_per_sequence, sequences_per_half_buffer,
      [this](const std::uint16_t* seq_ptr, std::uint32_t ts) noexcept {
        decoder_.ApplySequence(
            seq_ptr, midismith::adc_board::bsp::adc::AdcDma::kAdc3RanksPerSequence,
            ::midismith::adc_board::app::config::sensors::kAdc3SensorIdByRank, analog_group_, ts);
      });
}

void AnalogAcquisitionTask::ProcessFrame(
    const midismith::adc_board::bsp::adc::AdcFrameDescriptor& desc) noexcept {
  if (desc.group == midismith::adc_board::bsp::adc::AdcGroup::kAdc1) {
    ProcessAdc1Frame(desc);
    return;
  }
  if (desc.group == midismith::adc_board::bsp::adc::AdcGroup::kAdc2) {
    ProcessAdc2Frame(desc);
    return;
  }
  if (desc.group == midismith::adc_board::bsp::adc::AdcGroup::kAdc3) {
    ProcessAdc3Frame(desc);
  }
}

void AnalogAcquisitionTask::HandleEnabledState() noexcept {
  if (TryHandleDisableRequestWhileEnabled()) {
    return;
  }

  midismith::adc_board::bsp::adc::AdcFrameDescriptor desc{};
  if (!queue_.Receive(desc, 1)) {
    return;
  }
  ProcessFrame(desc);
}

void AnalogAcquisitionTask::run() noexcept {
  timestamp_counter_.Start();

  EnterDisabledState();

  TimestampCounterDelay delay(timestamp_counter_);
  midismith::adc_board::app::analog::AcquisitionSequencer sequencer(tia_shutdown_, delay, adc_dma_);

  for (;;) {
    if (state_ == midismith::adc_board::app::analog::AcquisitionState::kDisabled) {
      HandleDisabledState(sequencer);
      continue;
    }
    HandleEnabledState();
  }
}

bool AnalogAcquisitionTask::start() noexcept {
  return midismith::os::Task::create(
      "AnalogAcq", AnalogAcquisitionTask::entry, this,
      ::midismith::adc_board::app::config::ANALOG_ACQUISITION_TASK_STACK_BYTES,
      ::midismith::adc_board::app::config::ANALOG_ACQUISITION_TASK_PRIORITY);
}

}  // namespace midismith::adc_board::app::tasks
