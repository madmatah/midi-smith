#pragma once

#include <cstdint>
#include <type_traits>

#include "domain/dsp/converters/tia_current_converter.hpp"
#include "domain/dsp/engine/workflow.hpp"
#include "domain/dsp/filters/ema_filter.hpp"
#include "domain/dsp/filters/identity_filter.hpp"

namespace app::config {

constexpr bool SIGNAL_FILTERING_ENABLED = false;

constexpr std::int32_t SIGNAL_EMA_ALPHA_NUMERATOR = 1;
constexpr std::int32_t SIGNAL_EMA_ALPHA_DENOMINATOR = 8;

// Decimation factor is applied on segments of the pipeline to reduce processing frequency.
// Set to 1 to disable decimation.
constexpr std::uint8_t SIGNAL_DECIMATION_FACTOR = 1;

namespace signal_filtering_detail {

using FilteringEnabledPipeline = domain::dsp::engine::Workflow<
    domain::dsp::filters::EmaFilterRatio<SIGNAL_EMA_ALPHA_NUMERATOR, SIGNAL_EMA_ALPHA_DENOMINATOR>>;

using FilteringDisabledPipeline =
    domain::dsp::engine::Workflow<domain::dsp::filters::IdentityFilter>;

using FilteringPipeline =
    std::conditional_t<SIGNAL_FILTERING_ENABLED, signal_filtering_detail::FilteringEnabledPipeline,
                       signal_filtering_detail::FilteringDisabledPipeline>;

}  // namespace signal_filtering_detail

using AnalogSensorProcessor =
    domain::dsp::engine::Workflow<domain::dsp::converters::TiaCurrentConverter<2048, 16, 1800>,
                                  signal_filtering_detail::FilteringPipeline>;

}  // namespace app::config
