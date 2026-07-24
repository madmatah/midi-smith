#include "splash/animation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>
#include <string_view>

#include "splash/easing.hpp"
#include "splash/geometry.hpp"
#include "splash/palette.hpp"

namespace midismith::splash {
namespace {

constexpr double kNoteHeadTiltRadians = -0.42;

constexpr Point kHammerPivot{205.0, 42.0};
constexpr double kHammerBaseY = 42.0;
constexpr double kHammerHeadCenterX = 69.0;
constexpr double kHammerAssemblyScale = 1.35;
constexpr Point kHammerStrikePoint{69.0, 10.0};

constexpr Point kFeltCentroid{69.0, 22.0};

constexpr std::array<Point, 21> kFeltOutline{{
    {69.0, 10.0}, {65.0, 10.8}, {61.8, 12.9}, {59.5, 16.2}, {58.5, 20.0}, {58.7, 23.8},
    {59.8, 27.4}, {61.5, 30.6}, {63.5, 33.3}, {65.6, 35.5}, {67.3, 36.9}, {70.7, 36.9},
    {72.4, 35.5}, {74.5, 33.3}, {76.5, 30.6}, {78.2, 27.4}, {79.3, 23.8}, {79.5, 20.0},
    {78.5, 16.2}, {76.2, 12.9}, {73.0, 10.8},
}};

constexpr std::array<Point, 15> kUnderfeltOutline{{
    {69.0, 16.2},
    {70.5, 17.2},
    {71.5, 19.8},
    {72.0, 23.0},
    {72.2, 27.0},
    {72.1, 31.0},
    {71.8, 34.0},
    {71.6, 36.5},
    {66.4, 36.5},
    {66.2, 34.0},
    {65.9, 31.0},
    {65.8, 27.0},
    {66.0, 23.0},
    {66.5, 19.8},
    {67.5, 17.2},
}};

constexpr std::array<Point, 22> kMoldingBladeOutline{{
    {69.0, 18.2}, {70.1, 19.3}, {70.5, 22.5}, {70.6, 26.5}, {70.7, 30.5}, {70.7, 36.2},
    {72.3, 37.4}, {72.1, 39.4}, {71.1, 42.2}, {70.1, 45.0}, {69.0, 47.6}, {67.8, 49.6},
    {66.6, 48.3}, {66.3, 45.2}, {66.2, 41.8}, {66.4, 38.6}, {64.9, 37.5}, {67.3, 36.2},
    {67.3, 30.5}, {67.4, 26.5}, {67.5, 22.5}, {67.9, 19.3},
}};

constexpr std::array<Point, 3> kMoldingTailHighlight{{
    {66.5, 38.0},
    {66.3, 42.5},
    {67.1, 47.0},
}};

constexpr std::array<Point, 2> kShankOutline{{{70.5, kHammerBaseY}, {145.0, kHammerBaseY}}};
constexpr std::array<Point, 2> kShankHighlight{{{72.5, 40.9}, {145.0, 40.9}}};
constexpr std::array<Point, 2> kMoldingBladeCenter{{{69.0, 19.8}, {69.0, 35.5}}};

constexpr std::array<Point, 5> kFeltCrownArc = [] {
  std::array<Point, 5> arc{};
  for (std::size_t point_index = 0; point_index < arc.size(); ++point_index) {
    arc[point_index] = kFeltOutline[point_index];
  }
  return arc;
}();

constexpr double DegreesToRadians(double angle_degrees) {
  return angle_degrees * (std::numbers::pi_v<double> / 180.0);
}

constexpr Point ScaleHammerAssemblyPoint(Point point) {
  return {kHammerStrikePoint.x + (point.x - kHammerStrikePoint.x) * kHammerAssemblyScale,
          kHammerStrikePoint.y + (point.y - kHammerStrikePoint.y) * kHammerAssemblyScale};
}

double ImpactSquashAmount(double time_seconds) {
  const double rise = SmoothStep(PhaseProgress(time_seconds, 0.60, 0.655));
  const double fall = 1.0 - SmoothStep(PhaseProgress(time_seconds, 0.70, 0.79));
  return rise * fall;
}

double HammerAngleDegrees(double time_seconds) {
  if (time_seconds < 0.18) {
    return -7.2;
  }
  if (time_seconds < 0.64) {
    const double progress = PhaseProgress(time_seconds, 0.18, 0.64);
    return -7.2 + 7.2 * EaseInCubic(progress);
  }
  if (time_seconds < 0.70) {
    return 0.0;
  }
  const double progress = PhaseProgress(time_seconds, 0.70, 1.00);
  return -5.6 * EaseOutCubic(progress);
}

double HammerOpacity(double time_seconds) {
  const double fade_in = SmoothStep(PhaseProgress(time_seconds, 0.02, 0.20));
  const double fade_out = 1.0 - SmoothStep(PhaseProgress(time_seconds, 0.90, 1.08));
  return fade_in * fade_out;
}

template <std::size_t kPointCount>
std::array<Point, kPointCount> PlaceHammerPoints(const std::array<Point, kPointCount>& points,
                                                 double squash_amount, Point scaled_pivot,
                                                 double angle_radians) {
  const double horizontal_gain = 1.0 + 0.05 * squash_amount;
  const double vertical_gain = 1.0 - 0.03 * squash_amount;
  std::array<Point, kPointCount> placed{};
  for (std::size_t point_index = 0; point_index < kPointCount; ++point_index) {
    Point point = points[point_index];
    if (squash_amount > 0.0) {
      point = {kHammerHeadCenterX + (point.x - kHammerHeadCenterX) * horizontal_gain,
               kHammerBaseY - (kHammerBaseY - point.y) * vertical_gain};
    }
    placed[point_index] = RotatePoint(ScaleHammerAssemblyPoint(point), scaled_pivot, angle_radians);
  }
  return placed;
}

void DrawHammer(PixelCanvas& canvas, double angle_degrees, double opacity,
                double squash_amount = 0.0) {
  const double angle_radians = DegreesToRadians(angle_degrees);
  const Point scaled_pivot = ScaleHammerAssemblyPoint(kHammerPivot);

  const auto Place = [&](const auto& points) {
    return PlaceHammerPoints(points, squash_amount, scaled_pivot, angle_radians);
  };
  const auto PlaceWidth = [](double width) { return width * kHammerAssemblyScale; };

  const auto shank = Place(kShankOutline);
  canvas.DrawLine(shank[0].x, shank[0].y, shank[1].x, shank[1].y, PlaceWidth(3.4), kWood, opacity);
  const auto shank_highlight = Place(kShankHighlight);
  canvas.DrawLine(shank_highlight[0].x, shank_highlight[0].y, shank_highlight[1].x,
                  shank_highlight[1].y, PlaceWidth(0.7), kWoodHighlight, opacity * 0.75);

  canvas.FillPolygon(Place(kFeltOutline), kFeltShadow, opacity);
  const auto felt_inner = ScalePointsTowardCentroid(kFeltOutline, kFeltCentroid, 0.93);
  canvas.FillPolygon(Place(felt_inner), kFeltMidtone, opacity);

  canvas.FillPolygon(Place(kUnderfeltOutline), kUnderfeltInk, opacity);
  canvas.FillPolygon(Place(kMoldingBladeOutline), kWood, opacity);
  const auto blade_center = Place(kMoldingBladeCenter);
  canvas.DrawPolyline(blade_center, PlaceWidth(1.1), kWoodHighlight, opacity * 0.55);
  canvas.DrawPolyline(Place(kMoldingTailHighlight), PlaceWidth(0.5), kWoodHighlight, opacity * 0.7);

  const auto crown_highlight = ScalePointsTowardCentroid(kFeltCrownArc, kFeltCentroid, 0.97);
  canvas.DrawPolyline(Place(crown_highlight), PlaceWidth(0.6), kWarmIvory, opacity * 0.7);
}

void DrawHammerScene(PixelCanvas& canvas, double time_seconds) {
  const double opacity = HammerOpacity(time_seconds);
  if (opacity <= 0.0) {
    return;
  }
  DrawHammer(canvas, HammerAngleDegrees(time_seconds), opacity, ImpactSquashAmount(time_seconds));
}

double SignalBaselineY(double time_seconds) {
  const double progress = SmoothStep(PhaseProgress(time_seconds, 1.00, 1.28));
  return 10.0 + 25.0 * progress;
}

double ScrollOffsetX(double time_seconds) {
  return -27.0 * SmoothStep(PhaseProgress(time_seconds, 1.15, 1.95));
}

struct MidiBlock {
  double x;
  double vertical_offset;
  double width;
};

constexpr std::array<MidiBlock, 8> kMidiBlocks{{
    {57.0, -1.4, 5.5},
    {69.0, 1.4, 6.0},
    {82.0, -0.8, 4.5},
    {94.0, 1.0, 7.0},
    {108.0, -1.1, 5.5},
    {121.0, 0.8, 6.0},
    {135.0, -0.7, 5.0},
    {147.0, 0.9, 3.5},
}};

constexpr double kPlayheadSweepStartSeconds = 2.10;
constexpr double kPlayheadSweepEndSeconds = 3.00;

void DrawSignal(PixelCanvas& canvas, double time_seconds) {
  const double baseline_y = SignalBaselineY(time_seconds);
  if (time_seconds < 0.64) {
    canvas.DrawLine(0.0, baseline_y, static_cast<double>(kDisplayWidth), baseline_y, 0.5,
                    kStringIvory, 0.58);
    return;
  }

  const double scroll_offset_x = ScrollOffsetX(time_seconds);
  const double narrative_fade = 1.0 - SmoothStep(PhaseProgress(time_seconds, 3.05, 3.25));
  const double signal_progress = PhaseProgress(time_seconds, 0.64, 1.95);
  const double waveform_fade =
      (1.0 - SmoothStep(PhaseProgress(time_seconds, 1.62, 2.08))) * narrative_fade;
  const double wave_center_x = 69.0 + 38.0 * EaseOutCubic(signal_progress);
  const double wave_amplitude =
      6.0 * SmoothStep(PhaseProgress(time_seconds, 0.64, 0.73)) * (1.0 - 0.42 * signal_progress);
  std::array<Point, 161> wave_points{};
  for (int screen_index = 0; screen_index < 161; ++screen_index) {
    const double screen_x = static_cast<double>(screen_index);
    const double world_x = screen_x - scroll_offset_x;
    const double distance = world_x - wave_center_x;
    const double envelope = std::exp(-0.0019 * distance * distance);
    const double point_y = baseline_y + wave_amplitude * envelope * std::sin(distance * 0.82);
    wave_points[static_cast<std::size_t>(screen_index)] = {screen_x, point_y};
  }
  canvas.DrawPolyline(wave_points, 0.7, kWarmIvory, waveform_fade);

  const double digital_progress = SmoothStep(PhaseProgress(time_seconds, 1.18, 1.96));
  const double digital_fade =
      (1.0 - SmoothStep(PhaseProgress(time_seconds, 3.02, 3.22))) * narrative_fade;
  const double playhead_progress =
      PhaseProgress(time_seconds, kPlayheadSweepStartSeconds, kPlayheadSweepEndSeconds);
  const double playhead_world_x = 45.0 + 117.0 * playhead_progress;
  const bool playhead_active = 0.0 < playhead_progress && playhead_progress < 1.0;
  const int visible_block_count =
      static_cast<int>(std::ceil(static_cast<double>(kMidiBlocks.size()) * digital_progress));
  for (std::size_t block_index = 0; block_index < kMidiBlocks.size(); ++block_index) {
    if (static_cast<int>(block_index) >= visible_block_count) {
      break;
    }
    const MidiBlock& block = kMidiBlocks[block_index];
    const bool is_accent_block = block_index == 2 || block_index == 5;
    const Color block_color = is_accent_block ? kBrightIvory : kWarmIvory;
    double block_opacity =
        is_accent_block ? digital_fade : (0.58 + 0.42 * digital_progress) * digital_fade;
    double block_pulse = 0.0;
    if (playhead_active) {
      const double playhead_distance = block.x + block.width / 2.0 - playhead_world_x;
      block_pulse = std::exp(-(playhead_distance * playhead_distance) / 72.0);
    }
    block_opacity = block_opacity + (digital_fade - block_opacity) * block_pulse;
    const double block_lift = 1.2 * block_pulse;
    canvas.DrawLine(block.x + scroll_offset_x, baseline_y + block.vertical_offset - block_lift,
                    block.x + block.width + scroll_offset_x,
                    baseline_y + block.vertical_offset - block_lift, 1.4, block_color,
                    block_opacity);
  }
}

constexpr std::array<Point, 5> kImpactParticleVectors{{
    {-6.0, -4.2},
    {-2.0, -6.0},
    {3.3, -5.1},
    {6.2, -2.7},
    {5.5, 2.6},
}};

void DrawImpact(PixelCanvas& canvas, double time_seconds) {
  const double flash_progress = PhaseProgress(time_seconds, 0.635, 0.765);
  if (flash_progress <= 0.0 || flash_progress >= 1.0) {
    return;
  }
  const double opacity = std::sin(flash_progress * std::numbers::pi_v<double>);
  canvas.DrawDisc(69.0, 10.0, 6.6, kBrightIvory, opacity * 0.06);
  canvas.DrawDisc(69.0, 10.0, 3.0, kWarmIvory, opacity * 0.16);
  canvas.DrawDisc(69.0, 10.0, 0.95, kWarmIvory, opacity);
  for (std::size_t particle_index = 0; particle_index < kImpactParticleVectors.size();
       ++particle_index) {
    const Point direction = kImpactParticleVectors[particle_index];
    const double travel = EaseOutCubic(flash_progress) * 1.25;
    const double particle_x = 69.0 + direction.x * travel;
    const double particle_y = 10.0 + direction.y * travel + 2.2 * travel * travel;
    const double radius = particle_index < 3 ? 0.6 : 0.44;
    canvas.DrawDisc(particle_x, particle_y, radius, kBrightIvory,
                    opacity * (1.0 - 0.11 * static_cast<double>(particle_index)));
  }
}

void DrawNoteHead(PixelCanvas& canvas, double center_x, double center_y, double scale,
                  double growth, double opacity) {
  if (growth <= 0.0) {
    return;
  }
  canvas.DrawEllipse(center_x, center_y, 2.0 * scale * growth, 1.5 * scale * growth,
                     kNoteHeadTiltRadians, kWarmIvory, opacity);
}

void DrawEighthNote(PixelCanvas& canvas, double head_x, double head_y, double scale,
                    double progress, double opacity) {
  const double growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0));
  DrawNoteHead(canvas, head_x, head_y, scale, growth, opacity);
  const double stem_x = head_x + 1.6 * scale;
  const double stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0));
  const double stem_rise = 9.2 * scale * stem_progress;
  if (stem_rise > 0.5) {
    canvas.DrawLine(stem_x, head_y - 0.3 * scale, stem_x, head_y - stem_rise,
                    std::max(0.6, 0.55 * scale), kWarmIvory, opacity);
  }
  const double flag_progress = PhaseProgress(progress, 0.6, 1.0);
  if (flag_progress > 0.0) {
    const double stem_top = head_y - 9.2 * scale;
    const auto flag_points =
        BezierPoints<14>({stem_x, stem_top}, {stem_x + 3.4 * scale, stem_top + 1.3 * scale},
                         {stem_x + 3.9 * scale, stem_top + 3.6 * scale},
                         {stem_x + 1.7 * scale, stem_top + 5.6 * scale});
    canvas.DrawPartialPolyline(flag_points, EaseOutCubic(flag_progress), std::max(0.6, 0.5 * scale),
                               kWarmIvory, opacity);
  }
}

void DrawQuarterNoteStemDown(PixelCanvas& canvas, double head_x, double head_y, double scale,
                             double progress, double opacity) {
  const double growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0));
  DrawNoteHead(canvas, head_x, head_y, scale, growth, opacity);
  const double stem_x = head_x - 1.6 * scale;
  const double stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0));
  const double stem_drop = 9.6 * scale * stem_progress;
  if (stem_drop > 0.5) {
    canvas.DrawLine(stem_x, head_y + 0.3 * scale, stem_x, head_y + stem_drop,
                    std::max(0.6, 0.55 * scale), kWarmIvory, opacity);
  }
}

void DrawBeamedEighthPair(PixelCanvas& canvas, double head_x, double head_y, double scale,
                          double progress, double opacity) {
  const double second_x = head_x + 5.6 * scale;
  const double second_y = head_y - 1.5 * scale;
  const double first_growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0));
  const double second_growth = SmoothStep(Clamp((progress - 0.08) * 2.4, 0.0, 1.0));
  DrawNoteHead(canvas, head_x, head_y, scale, first_growth, opacity);
  DrawNoteHead(canvas, second_x, second_y, scale, second_growth, opacity);
  const double stem_height = 8.8 * scale;
  const double first_stem_x = head_x + 1.6 * scale;
  const double second_stem_x = second_x + 1.6 * scale;
  const double first_stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0));
  const double second_stem_progress = SmoothStep(Clamp((progress - 0.08) * 1.7, 0.0, 1.0));
  if (first_stem_progress > 0.05) {
    canvas.DrawLine(first_stem_x, head_y - 0.3 * scale, first_stem_x,
                    head_y - stem_height * first_stem_progress, std::max(0.6, 0.55 * scale),
                    kWarmIvory, opacity);
  }
  if (second_stem_progress > 0.05) {
    canvas.DrawLine(second_stem_x, second_y - 0.3 * scale, second_stem_x,
                    second_y - stem_height * second_stem_progress, std::max(0.6, 0.55 * scale),
                    kWarmIvory, opacity);
  }
  const double beam_progress = EaseOutCubic(PhaseProgress(progress, 0.6, 1.0));
  if (beam_progress > 0.0) {
    const Point beam_start{first_stem_x, head_y - stem_height};
    const Point beam_end{second_stem_x, second_y - stem_height};
    const double beam_tip_x = beam_start.x + (beam_end.x - beam_start.x) * beam_progress;
    const double beam_tip_y = beam_start.y + (beam_end.y - beam_start.y) * beam_progress;
    canvas.DrawLine(beam_start.x, beam_start.y, beam_tip_x, beam_tip_y, std::max(0.8, 1.05 * scale),
                    kWarmIvory, opacity);
  }
}

enum class EmergingNoteKind { kEighth, kQuarterStemDown, kBeamedPair };

struct EmergingNoteSpec {
  EmergingNoteKind kind;
  double x;
  double start_seconds;
  double scale;
  double rise;
  double sway_phase;
};

constexpr std::array<EmergingNoteSpec, 3> kEmergingNoteSpecs{{
    {EmergingNoteKind::kEighth, 82.0, 1.60, 0.95, 8.0, 0.0},
    {EmergingNoteKind::kQuarterStemDown, 108.0, 1.78, 1.18, 13.0, 2.1},
    {EmergingNoteKind::kBeamedPair, 131.0, 1.96, 0.78, 5.5, 4.2},
}};

constexpr double kNoteEmergenceDurationSeconds = 0.45;

void DrawEmergingNotes(PixelCanvas& canvas, double time_seconds) {
  const double narrative_opacity = 1.0 - SmoothStep(PhaseProgress(time_seconds, 3.05, 3.25));
  if (narrative_opacity <= 0.0) {
    return;
  }
  const double baseline_y = 35.0;
  const double scroll_offset_x = ScrollOffsetX(time_seconds);
  for (const EmergingNoteSpec& spec : kEmergingNoteSpecs) {
    const double progress = PhaseProgress(time_seconds, spec.start_seconds,
                                          spec.start_seconds + kNoteEmergenceDurationSeconds);
    if (progress <= 0.0) {
      continue;
    }
    const double settled = SmoothStep(progress);
    const double float_drift = 2.5 * Clamp(time_seconds - spec.start_seconds, 0.0, 1.0);
    const double bob = 0.7 * std::sin(5.0 * time_seconds + spec.sway_phase) * settled;
    const double playhead_progress =
        PhaseProgress(time_seconds, kPlayheadSweepStartSeconds, kPlayheadSweepEndSeconds);
    double note_pop = 0.0;
    if (0.0 < playhead_progress && playhead_progress < 1.0) {
      const double playhead_distance = spec.x - (45.0 + 117.0 * playhead_progress);
      note_pop = std::exp(-(playhead_distance * playhead_distance) / 72.0);
    }
    const double head_y =
        baseline_y - 3.0 - spec.rise * EaseOutCubic(progress) - float_drift + bob - 1.0 * note_pop;
    const double sway = 1.1 * std::sin(2.4 * time_seconds + spec.sway_phase) * settled;
    const double screen_x = spec.x + scroll_offset_x + sway;
    const double popped_scale = spec.scale * (1.0 + 0.15 * note_pop);
    const double opacity = narrative_opacity * SmoothStep(Clamp(progress * 3.0, 0.0, 1.0));
    switch (spec.kind) {
      case EmergingNoteKind::kEighth:
        DrawEighthNote(canvas, screen_x, head_y, popped_scale, progress, opacity);
        break;
      case EmergingNoteKind::kQuarterStemDown:
        DrawQuarterNoteStemDown(canvas, screen_x, head_y, popped_scale, progress, opacity);
        break;
      case EmergingNoteKind::kBeamedPair:
        DrawBeamedEighthPair(canvas, screen_x, head_y, popped_scale, progress, opacity);
        break;
    }
  }
}

constexpr std::array<Point, 5> kGlyphMStroke{{
    {0.0, 7.0},
    {0.0, 0.0},
    {2.3, 4.3},
    {4.6, 0.0},
    {4.6, 7.0},
}};
constexpr std::array<Point, 2> kGlyphIStroke{{{0.5, 0.0}, {0.5, 7.0}}};
constexpr std::array<Point, 2> kGlyphDStem{{{0.0, 0.0}, {0.0, 7.0}}};
constexpr std::array<Point, 13> kGlyphDBowl =
    BezierPoints<12>({0.0, 0.0}, {4.4, 0.2}, {4.4, 6.8}, {0.0, 7.0});
constexpr std::array<Point, 15> kGlyphSStroke{{
    {3.3, 0.95},
    {2.7, 0.25},
    {1.6, 0.05},
    {0.7, 0.45},
    {0.25, 1.25},
    {0.35, 2.15},
    {1.0, 2.9},
    {2.3, 3.55},
    {3.3, 4.25},
    {3.6, 5.15},
    {3.45, 6.05},
    {2.8, 6.7},
    {1.7, 6.95},
    {0.7, 6.65},
    {0.2, 5.9},
}};
constexpr std::array<Point, 2> kGlyphTBar{{{0.0, 0.0}, {4.0, 0.0}}};
constexpr std::array<Point, 2> kGlyphTStem{{{2.0, 0.0}, {2.0, 7.0}}};
constexpr std::array<Point, 2> kGlyphHLeftStem{{{0.0, 0.0}, {0.0, 7.0}}};
constexpr std::array<Point, 2> kGlyphHRightStem{{{4.2, 0.0}, {4.2, 7.0}}};
constexpr std::array<Point, 2> kGlyphHBar{{{0.0, 3.6}, {4.2, 3.6}}};

constexpr std::array<std::span<const Point>, 1> kGlyphMStrokes{
    std::span<const Point>{kGlyphMStroke}};
constexpr std::array<std::span<const Point>, 1> kGlyphIStrokes{
    std::span<const Point>{kGlyphIStroke}};
constexpr std::array<std::span<const Point>, 2> kGlyphDStrokes{std::span<const Point>{kGlyphDStem},
                                                               std::span<const Point>{kGlyphDBowl}};
constexpr std::array<std::span<const Point>, 1> kGlyphSStrokes{
    std::span<const Point>{kGlyphSStroke}};
constexpr std::array<std::span<const Point>, 2> kGlyphTStrokes{std::span<const Point>{kGlyphTBar},
                                                               std::span<const Point>{kGlyphTStem}};
constexpr std::array<std::span<const Point>, 3> kGlyphHStrokes{
    std::span<const Point>{kGlyphHLeftStem}, std::span<const Point>{kGlyphHRightStem},
    std::span<const Point>{kGlyphHBar}};

struct WordmarkGlyph {
  double advance;
  std::span<const std::span<const Point>> strokes;
};

constexpr WordmarkGlyph kGlyphM{4.6, kGlyphMStrokes};
constexpr WordmarkGlyph kGlyphI{1.0, kGlyphIStrokes};
constexpr WordmarkGlyph kGlyphD{3.5, kGlyphDStrokes};
constexpr WordmarkGlyph kGlyphS{3.7, kGlyphSStrokes};
constexpr WordmarkGlyph kGlyphT{4.0, kGlyphTStrokes};
constexpr WordmarkGlyph kGlyphH{4.2, kGlyphHStrokes};
constexpr WordmarkGlyph kGlyphSpace{2.4, {}};

constexpr const WordmarkGlyph& GlyphFor(char character) {
  switch (character) {
    case 'M':
      return kGlyphM;
    case 'I':
      return kGlyphI;
    case 'D':
      return kGlyphD;
    case 'S':
      return kGlyphS;
    case 'T':
      return kGlyphT;
    case 'H':
      return kGlyphH;
    default:
      return kGlyphSpace;
  }
}

constexpr std::string_view kWordmarkText = "MIDI SMITH";
constexpr double kWordmarkScale = 2.25;
constexpr double kWordmarkLetterSpacingUnits = 1.5;
constexpr double kWordmarkOriginX = 45.5;
constexpr double kWordmarkOriginY = 30.0;
constexpr double kWordmarkStrokeWidth = 0.62 * kWordmarkScale;
constexpr double kWordmarkRevealStartSeconds = 3.51;
constexpr double kWordmarkLetterStaggerSeconds = 0.05;
constexpr double kWordmarkLetterFadeSeconds = 0.12;
constexpr double kWordmarkShineStartSeconds = 4.20;
constexpr double kWordmarkShineEndSeconds = 4.55;

constexpr Point kFinalNoteHeadCenter{25.0, 51.0};
constexpr double kFinalNoteScale = 2.7;

constexpr double kRuleY = 51.0;
constexpr double kRuleSweepStartSeconds = 3.93;
constexpr double kRuleSweepEndSeconds = 4.19;
constexpr double kRuleTipFadeStartSeconds = 4.19;
constexpr double kRuleTipFadeEndSeconds = 4.33;

constexpr double MeasureWordmarkWidth() {
  double advance_sum = 0.0;
  for (const char character : kWordmarkText) {
    advance_sum += GlyphFor(character).advance;
  }
  const double spacing_sum =
      kWordmarkLetterSpacingUnits * static_cast<double>(kWordmarkText.size() - 1);
  return (advance_sum + spacing_sum) * kWordmarkScale;
}

void DrawFinalNote(PixelCanvas& canvas, double time_seconds, double exit_opacity = 1.0) {
  const double head_progress = SmoothStep(PhaseProgress(time_seconds, 3.33, 3.49));
  if (head_progress <= 0.0) {
    return;
  }
  const double stem_progress = SmoothStep(PhaseProgress(time_seconds, 3.37, 3.57));
  const double flag_progress = PhaseProgress(time_seconds, 3.53, 3.75);
  const double scale = kFinalNoteScale;
  const double head_x = kFinalNoteHeadCenter.x;
  const double head_y = kFinalNoteHeadCenter.y;
  canvas.DrawEllipse(head_x, head_y, 2.0 * scale * head_progress, 1.5 * scale * head_progress,
                     kNoteHeadTiltRadians, kWarmIvory,
                     Clamp(head_progress * 1.6, 0.0, 1.0) * exit_opacity);
  const double stem_x = head_x + 1.6 * scale;
  const double stem_bottom = head_y - 0.3 * scale;
  const double stem_top_full = head_y - 11.3 * scale;
  if (stem_progress > 0.0) {
    const double stem_top = stem_bottom - (stem_bottom - stem_top_full) * stem_progress;
    canvas.DrawLine(stem_x, stem_bottom, stem_x, stem_top, 1.8, kWarmIvory, exit_opacity);
  }
  if (flag_progress > 0.0) {
    const auto outer_points = BezierPoints<16>({stem_x, stem_top_full},
                                               {stem_x + 4.5 * scale, stem_top_full + 1.6 * scale},
                                               {stem_x + 5.0 * scale, stem_top_full + 4.6 * scale},
                                               {stem_x + 2.1 * scale, stem_top_full + 7.2 * scale});
    const auto inner_points = BezierPoints<16>({stem_x, stem_top_full + 1.0 * scale},
                                               {stem_x + 3.4 * scale, stem_top_full + 2.2 * scale},
                                               {stem_x + 3.9 * scale, stem_top_full + 4.4 * scale},
                                               {stem_x + 1.9 * scale, stem_top_full + 6.9 * scale});
    const auto revealed_count =
        std::max(std::size_t{2},
                 static_cast<std::size_t>(std::nearbyint(EaseOutCubic(flag_progress) * 16.0)) + 1);
    std::array<Point, 34> flag_polygon{};
    for (std::size_t point_index = 0; point_index < revealed_count; ++point_index) {
      flag_polygon[point_index] = outer_points[point_index];
      flag_polygon[revealed_count + point_index] = inner_points[revealed_count - 1 - point_index];
    }
    canvas.FillPolygon(std::span<const Point>(flag_polygon).first(2 * revealed_count), kWarmIvory,
                       exit_opacity);
  }
}

void DrawWordmark(PixelCanvas& canvas, double time_seconds, double exit_opacity = 1.0) {
  const double shine_progress =
      PhaseProgress(time_seconds, kWordmarkShineStartSeconds, kWordmarkShineEndSeconds);
  const double shine_x = 40.0 + 110.0 * shine_progress;
  const bool shine_active = 0.0 < shine_progress && shine_progress < 1.0;
  double current_x = kWordmarkOriginX;
  int drawn_glyph_index = 0;
  for (const char character : kWordmarkText) {
    const WordmarkGlyph& glyph = GlyphFor(character);
    if (!glyph.strokes.empty()) {
      const double reveal_start =
          kWordmarkRevealStartSeconds +
          kWordmarkLetterStaggerSeconds * static_cast<double>(drawn_glyph_index);
      const double reveal = SmoothStep(
          PhaseProgress(time_seconds, reveal_start, reveal_start + kWordmarkLetterFadeSeconds));
      if (reveal > 0.0) {
        const double settle_offset = 1.4 * (1.0 - EaseOutCubic(reveal));
        double shine_weight = 0.0;
        if (shine_active) {
          const double glyph_center_x = current_x + glyph.advance * kWordmarkScale / 2.0;
          const double shine_distance = glyph_center_x - shine_x;
          shine_weight = std::exp(-(shine_distance * shine_distance) / 50.0);
        }
        for (const std::span<const Point> stroke : glyph.strokes) {
          std::array<Point, 16> placed{};
          for (std::size_t point_index = 0; point_index < stroke.size(); ++point_index) {
            placed[point_index] = {
                current_x + stroke[point_index].x * kWordmarkScale,
                kWordmarkOriginY + settle_offset + stroke[point_index].y * kWordmarkScale};
          }
          canvas.DrawPolyline(std::span<const Point>(placed).first(stroke.size()),
                              kWordmarkStrokeWidth, kWarmIvory, reveal * exit_opacity);
          if (shine_weight > 0.02) {
            canvas.DrawPolyline(std::span<const Point>(placed).first(stroke.size()),
                                kWordmarkStrokeWidth * 1.7, kBrightIvory,
                                reveal * 0.75 * shine_weight * exit_opacity);
          }
        }
      }
      ++drawn_glyph_index;
    }
    current_x += (glyph.advance + kWordmarkLetterSpacingUnits) * kWordmarkScale;
  }
}

void DrawBaselineRule(PixelCanvas& canvas, double time_seconds, double exit_opacity = 1.0) {
  const double sweep =
      EaseOutCubic(PhaseProgress(time_seconds, kRuleSweepStartSeconds, kRuleSweepEndSeconds));
  if (sweep <= 0.0) {
    return;
  }
  const double rule_start_x = kWordmarkOriginX;
  const double rule_end_x = kWordmarkOriginX + MeasureWordmarkWidth();
  const double tip_x = rule_start_x + (rule_end_x - rule_start_x) * sweep;
  canvas.DrawLine(rule_start_x, kRuleY, tip_x, kRuleY, 0.55, kStringIvory, 0.6 * exit_opacity);
  const double tip_fade = 1.0 - SmoothStep(PhaseProgress(time_seconds, kRuleTipFadeStartSeconds,
                                                         kRuleTipFadeEndSeconds));
  if (tip_fade > 0.0) {
    canvas.DrawDisc(tip_x, kRuleY, 2.4, kBrightIvory, 0.10 * tip_fade * exit_opacity);
    canvas.DrawDisc(tip_x, kRuleY, 0.85, kBrightIvory, 0.85 * tip_fade * exit_opacity);
  }
}

constexpr double kExitFadeStartSeconds = 4.75;
constexpr double kExitFadeEndSeconds = 4.93;

void DrawFinalIdentity(PixelCanvas& canvas, double time_seconds) {
  const double exit_opacity =
      1.0 - SmoothStep(PhaseProgress(time_seconds, kExitFadeStartSeconds, kExitFadeEndSeconds));
  if (exit_opacity <= 0.0) {
    return;
  }
  DrawFinalNote(canvas, time_seconds, exit_opacity);
  DrawWordmark(canvas, time_seconds, exit_opacity);
  DrawBaselineRule(canvas, time_seconds, exit_opacity);
}

}  // namespace

void RenderFrame(double time_seconds, PixelCanvas& canvas) noexcept {
  canvas.Clear();
  DrawSignal(canvas, time_seconds);
  DrawHammerScene(canvas, time_seconds);
  DrawImpact(canvas, time_seconds);
  DrawEmergingNotes(canvas, time_seconds);
  DrawFinalIdentity(canvas, time_seconds);
}

}  // namespace midismith::splash
