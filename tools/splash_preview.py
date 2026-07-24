#!/usr/bin/env python3

import argparse
import math
import shutil
import subprocess
import tempfile
from pathlib import Path


DISPLAY_WIDTH = 160
DISPLAY_HEIGHT = 80
FRAME_RATE_HZ = 30
ANIMATION_DURATION_SECONDS = 4.75
SUPERSAMPLING_SCALE = 4
SPLASH_VERSION = "v3"

BLACK = (0, 0, 0)
GRAPHITE = (48, 45, 42)
STRING_IVORY = (180, 173, 160)
WARM_IVORY = (244, 237, 222)
FELT_MIDTONE = (202, 190, 169)
FELT_SHADOW = (82, 73, 63)
WOOD = (105, 70, 43)
WOOD_HIGHLIGHT = (165, 118, 73)
UNDERFELT_INK = (52, 56, 74)
BRIGHT_IVORY = (255, 252, 244)

NOTE_HEAD_TILT_RADIANS = -0.42


def Clamp(value, minimum, maximum):
    return max(minimum, min(maximum, value))


def SmoothStep(progress):
    progress = Clamp(progress, 0.0, 1.0)
    return progress * progress * (3.0 - 2.0 * progress)


def EaseInCubic(progress):
    progress = Clamp(progress, 0.0, 1.0)
    return progress * progress * progress


def EaseOutCubic(progress):
    progress = Clamp(progress, 0.0, 1.0)
    return 1.0 - (1.0 - progress) ** 3


def PhaseProgress(time_seconds, start_seconds, end_seconds):
    if end_seconds <= start_seconds:
        return 1.0
    return Clamp(
        (time_seconds - start_seconds) / (end_seconds - start_seconds),
        0.0,
        1.0,
    )


def BezierPoints(start, first_control, second_control, end, segment_count=16):
    points = []
    for segment_index in range(segment_count + 1):
        progress = segment_index / segment_count
        inverse_progress = 1.0 - progress
        point_x = (
            inverse_progress**3 * start[0]
            + 3.0 * inverse_progress**2 * progress * first_control[0]
            + 3.0 * inverse_progress * progress**2 * second_control[0]
            + progress**3 * end[0]
        )
        point_y = (
            inverse_progress**3 * start[1]
            + 3.0 * inverse_progress**2 * progress * first_control[1]
            + 3.0 * inverse_progress * progress**2 * second_control[1]
            + progress**3 * end[1]
        )
        points.append((point_x, point_y))
    return points


class Canvas:
    def __init__(self, width, height, scale=SUPERSAMPLING_SCALE):
        self.width = width
        self.height = height
        self.scale = scale
        self.scaled_width = width * scale
        self.scaled_height = height * scale
        self.pixels = bytearray(self.scaled_width * self.scaled_height * 3)

    def BlendPixel(self, pixel_x, pixel_y, color, opacity):
        if (
            pixel_x < 0
            or pixel_x >= self.scaled_width
            or pixel_y < 0
            or pixel_y >= self.scaled_height
            or opacity <= 0.0
        ):
            return
        opacity = Clamp(opacity, 0.0, 1.0)
        offset = (pixel_y * self.scaled_width + pixel_x) * 3
        inverse_opacity = 1.0 - opacity
        self.pixels[offset] = round(
            self.pixels[offset] * inverse_opacity + color[0] * opacity
        )
        self.pixels[offset + 1] = round(
            self.pixels[offset + 1] * inverse_opacity + color[1] * opacity
        )
        self.pixels[offset + 2] = round(
            self.pixels[offset + 2] * inverse_opacity + color[2] * opacity
        )

    def DrawDisc(self, center_x, center_y, radius, color, opacity=1.0):
        scaled_center_x = center_x * self.scale
        scaled_center_y = center_y * self.scale
        scaled_radius = radius * self.scale
        minimum_x = math.floor(scaled_center_x - scaled_radius)
        maximum_x = math.ceil(scaled_center_x + scaled_radius)
        minimum_y = math.floor(scaled_center_y - scaled_radius)
        maximum_y = math.ceil(scaled_center_y + scaled_radius)
        radius_squared = scaled_radius * scaled_radius
        for pixel_y in range(minimum_y, maximum_y + 1):
            distance_y = pixel_y + 0.5 - scaled_center_y
            for pixel_x in range(minimum_x, maximum_x + 1):
                distance_x = pixel_x + 0.5 - scaled_center_x
                if distance_x * distance_x + distance_y * distance_y <= radius_squared:
                    self.BlendPixel(pixel_x, pixel_y, color, opacity)

    def DrawEllipse(
        self,
        center_x,
        center_y,
        radius_x,
        radius_y,
        tilt_radians,
        color,
        opacity=1.0,
    ):
        if radius_x <= 0.0 or radius_y <= 0.0:
            return
        scaled_center_x = center_x * self.scale
        scaled_center_y = center_y * self.scale
        scaled_radius_x = radius_x * self.scale
        scaled_radius_y = radius_y * self.scale
        extent = max(scaled_radius_x, scaled_radius_y)
        minimum_x = math.floor(scaled_center_x - extent)
        maximum_x = math.ceil(scaled_center_x + extent)
        minimum_y = math.floor(scaled_center_y - extent)
        maximum_y = math.ceil(scaled_center_y + extent)
        cosine = math.cos(tilt_radians)
        sine = math.sin(tilt_radians)
        for pixel_y in range(minimum_y, maximum_y + 1):
            distance_y = pixel_y + 0.5 - scaled_center_y
            for pixel_x in range(minimum_x, maximum_x + 1):
                distance_x = pixel_x + 0.5 - scaled_center_x
                local_x = cosine * distance_x + sine * distance_y
                local_y = -sine * distance_x + cosine * distance_y
                normalized = (local_x / scaled_radius_x) ** 2 + (
                    local_y / scaled_radius_y
                ) ** 2
                if normalized <= 1.0:
                    self.BlendPixel(pixel_x, pixel_y, color, opacity)

    def DrawLine(
        self,
        start_x,
        start_y,
        end_x,
        end_y,
        width,
        color,
        opacity=1.0,
    ):
        scaled_start_x = start_x * self.scale
        scaled_start_y = start_y * self.scale
        scaled_end_x = end_x * self.scale
        scaled_end_y = end_y * self.scale
        scaled_radius = width * self.scale / 2.0
        minimum_x = math.floor(min(scaled_start_x, scaled_end_x) - scaled_radius)
        maximum_x = math.ceil(max(scaled_start_x, scaled_end_x) + scaled_radius)
        minimum_y = math.floor(min(scaled_start_y, scaled_end_y) - scaled_radius)
        maximum_y = math.ceil(max(scaled_start_y, scaled_end_y) + scaled_radius)
        direction_x = scaled_end_x - scaled_start_x
        direction_y = scaled_end_y - scaled_start_y
        length_squared = direction_x * direction_x + direction_y * direction_y
        for pixel_y in range(minimum_y, maximum_y + 1):
            sample_y = pixel_y + 0.5
            for pixel_x in range(minimum_x, maximum_x + 1):
                sample_x = pixel_x + 0.5
                if length_squared == 0.0:
                    projection = 0.0
                else:
                    projection = (
                        (sample_x - scaled_start_x) * direction_x
                        + (sample_y - scaled_start_y) * direction_y
                    ) / length_squared
                projection = Clamp(projection, 0.0, 1.0)
                nearest_x = scaled_start_x + projection * direction_x
                nearest_y = scaled_start_y + projection * direction_y
                distance_x = sample_x - nearest_x
                distance_y = sample_y - nearest_y
                if (
                    distance_x * distance_x + distance_y * distance_y
                    <= scaled_radius * scaled_radius
                ):
                    self.BlendPixel(pixel_x, pixel_y, color, opacity)

    def DrawPolyline(self, points, width, color, opacity=1.0):
        for point_index in range(len(points) - 1):
            self.DrawLine(
                points[point_index][0],
                points[point_index][1],
                points[point_index + 1][0],
                points[point_index + 1][1],
                width,
                color,
                opacity,
            )

    def DrawPartialPolyline(self, points, fraction, width, color, opacity=1.0):
        fraction = Clamp(fraction, 0.0, 1.0)
        if fraction <= 0.0 or len(points) < 2:
            return
        last_index = max(1, round(fraction * (len(points) - 1)))
        self.DrawPolyline(points[: last_index + 1], width, color, opacity)

    def FillPolygon(self, points, color, opacity=1.0):
        scaled_points = [
            (point_x * self.scale, point_y * self.scale)
            for point_x, point_y in points
        ]
        minimum_y = max(0, math.floor(min(point[1] for point in scaled_points)))
        maximum_y = min(
            self.scaled_height - 1,
            math.ceil(max(point[1] for point in scaled_points)),
        )
        for pixel_y in range(minimum_y, maximum_y + 1):
            scan_y = pixel_y + 0.5
            intersections = []
            for point_index, first_point in enumerate(scaled_points):
                second_point = scaled_points[(point_index + 1) % len(scaled_points)]
                first_x, first_y = first_point
                second_x, second_y = second_point
                if (first_y <= scan_y < second_y) or (second_y <= scan_y < first_y):
                    interpolation = (scan_y - first_y) / (second_y - first_y)
                    intersections.append(first_x + interpolation * (second_x - first_x))
            intersections.sort()
            for intersection_index in range(0, len(intersections) - 1, 2):
                minimum_x = max(0, math.ceil(intersections[intersection_index] - 0.5))
                maximum_x = min(
                    self.scaled_width - 1,
                    math.floor(intersections[intersection_index + 1] - 0.5),
                )
                for pixel_x in range(minimum_x, maximum_x + 1):
                    self.BlendPixel(pixel_x, pixel_y, color, opacity)

    def Downsample(self):
        output = bytearray(self.width * self.height * 3)
        sample_count = self.scale * self.scale
        for output_y in range(self.height):
            for output_x in range(self.width):
                red_sum = 0
                green_sum = 0
                blue_sum = 0
                for sample_y in range(self.scale):
                    source_y = output_y * self.scale + sample_y
                    for sample_x in range(self.scale):
                        source_x = output_x * self.scale + sample_x
                        source_offset = (
                            source_y * self.scaled_width + source_x
                        ) * 3
                        red_sum += self.pixels[source_offset]
                        green_sum += self.pixels[source_offset + 1]
                        blue_sum += self.pixels[source_offset + 2]
                output_offset = (output_y * self.width + output_x) * 3
                output[output_offset] = red_sum // sample_count
                output[output_offset + 1] = green_sum // sample_count
                output[output_offset + 2] = blue_sum // sample_count
        return bytes(output)


def RotatePoint(point, pivot, angle_radians):
    point_x, point_y = point
    pivot_x, pivot_y = pivot
    direction_x = point_x - pivot_x
    direction_y = point_y - pivot_y
    cosine = math.cos(angle_radians)
    sine = math.sin(angle_radians)
    return (
        pivot_x + cosine * direction_x - sine * direction_y,
        pivot_y + sine * direction_x + cosine * direction_y,
    )


def TransformPoints(points, pivot, angle_radians):
    return [RotatePoint(point, pivot, angle_radians) for point in points]


def ScalePointsTowardCentroid(points, centroid, factor):
    centroid_x, centroid_y = centroid
    return [
        (
            centroid_x + (point_x - centroid_x) * factor,
            centroid_y + (point_y - centroid_y) * factor,
        )
        for point_x, point_y in points
    ]


HAMMER_PIVOT = (205.0, 42.0)
HAMMER_BASE_Y = 42.0
HAMMER_HEAD_CENTER_X = 69.0
HAMMER_ASSEMBLY_SCALE = 1.35
HAMMER_STRIKE_POINT = (69.0, 10.0)


def ScaleHammerAssemblyPoint(point):
    return (
        HAMMER_STRIKE_POINT[0]
        + (point[0] - HAMMER_STRIKE_POINT[0]) * HAMMER_ASSEMBLY_SCALE,
        HAMMER_STRIKE_POINT[1]
        + (point[1] - HAMMER_STRIKE_POINT[1]) * HAMMER_ASSEMBLY_SCALE,
    )
FELT_CENTROID = (69.0, 22.0)

FELT_OUTLINE = [
    (69.0, 10.0),
    (65.0, 10.8),
    (61.8, 12.9),
    (59.5, 16.2),
    (58.5, 20.0),
    (58.7, 23.8),
    (59.8, 27.4),
    (61.5, 30.6),
    (63.5, 33.3),
    (65.6, 35.5),
    (67.3, 36.9),
    (70.7, 36.9),
    (72.4, 35.5),
    (74.5, 33.3),
    (76.5, 30.6),
    (78.2, 27.4),
    (79.3, 23.8),
    (79.5, 20.0),
    (78.5, 16.2),
    (76.2, 12.9),
    (73.0, 10.8),
]

UNDERFELT_OUTLINE = [
    (69.0, 16.2),
    (70.5, 17.2),
    (71.5, 19.8),
    (72.0, 23.0),
    (72.2, 27.0),
    (72.1, 31.0),
    (71.8, 34.0),
    (71.6, 36.5),
    (66.4, 36.5),
    (66.2, 34.0),
    (65.9, 31.0),
    (65.8, 27.0),
    (66.0, 23.0),
    (66.5, 19.8),
    (67.5, 17.2),
]

MOLDING_BLADE_OUTLINE = [
    (69.0, 18.2),
    (70.1, 19.3),
    (70.5, 22.5),
    (70.6, 26.5),
    (70.7, 30.5),
    (70.7, 36.2),
    (72.3, 37.4),
    (72.1, 39.4),
    (71.1, 42.2),
    (70.1, 45.0),
    (69.0, 47.6),
    (67.8, 49.6),
    (66.6, 48.3),
    (66.3, 45.2),
    (66.2, 41.8),
    (66.4, 38.6),
    (64.9, 37.5),
    (67.3, 36.2),
    (67.3, 30.5),
    (67.4, 26.5),
    (67.5, 22.5),
    (67.9, 19.3),
]

MOLDING_TAIL_HIGHLIGHT = [
    (66.5, 38.0),
    (66.3, 42.5),
    (67.1, 47.0),
]


def ImpactSquashAmount(time_seconds):
    rise = SmoothStep(PhaseProgress(time_seconds, 0.60, 0.655))
    fall = 1.0 - SmoothStep(PhaseProgress(time_seconds, 0.70, 0.79))
    return rise * fall


def SquashHammerPoints(points, squash_amount):
    if squash_amount <= 0.0:
        return points
    horizontal_gain = 1.0 + 0.05 * squash_amount
    vertical_gain = 1.0 - 0.03 * squash_amount
    return [
        (
            HAMMER_HEAD_CENTER_X + (point_x - HAMMER_HEAD_CENTER_X) * horizontal_gain,
            HAMMER_BASE_Y - (HAMMER_BASE_Y - point_y) * vertical_gain,
        )
        for point_x, point_y in points
    ]


def HammerAngleDegrees(time_seconds):
    if time_seconds < 0.18:
        return -7.2
    if time_seconds < 0.64:
        progress = PhaseProgress(time_seconds, 0.18, 0.64)
        return -7.2 + 7.2 * EaseInCubic(progress)
    if time_seconds < 0.70:
        return 0.0
    progress = PhaseProgress(time_seconds, 0.70, 1.00)
    return -5.6 * EaseOutCubic(progress)


def HammerOpacity(time_seconds):
    fade_in = SmoothStep(PhaseProgress(time_seconds, 0.02, 0.20))
    fade_out = 1.0 - SmoothStep(PhaseProgress(time_seconds, 0.90, 1.08))
    return fade_in * fade_out


def DrawHammer(canvas, angle_degrees, opacity, squash_amount=0.0):
    angle_radians = math.radians(angle_degrees)
    scaled_pivot = ScaleHammerAssemblyPoint(HAMMER_PIVOT)

    def PlacePoints(points):
        squashed = SquashHammerPoints(points, squash_amount)
        scaled = [ScaleHammerAssemblyPoint(point) for point in squashed]
        return TransformPoints(scaled, scaled_pivot, angle_radians)

    def PlaceWidth(width):
        return width * HAMMER_ASSEMBLY_SCALE

    shank = PlacePoints([(70.5, HAMMER_BASE_Y), (145.0, HAMMER_BASE_Y)])
    canvas.DrawLine(
        shank[0][0],
        shank[0][1],
        shank[1][0],
        shank[1][1],
        PlaceWidth(3.4),
        WOOD,
        opacity,
    )
    shank_highlight = PlacePoints([(72.5, 40.9), (145.0, 40.9)])
    canvas.DrawLine(
        shank_highlight[0][0],
        shank_highlight[0][1],
        shank_highlight[1][0],
        shank_highlight[1][1],
        PlaceWidth(0.7),
        WOOD_HIGHLIGHT,
        opacity * 0.75,
    )

    canvas.FillPolygon(PlacePoints(FELT_OUTLINE), FELT_SHADOW, opacity)
    felt_inner = ScalePointsTowardCentroid(FELT_OUTLINE, FELT_CENTROID, 0.93)
    canvas.FillPolygon(PlacePoints(felt_inner), FELT_MIDTONE, opacity)

    canvas.FillPolygon(PlacePoints(UNDERFELT_OUTLINE), UNDERFELT_INK, opacity)
    canvas.FillPolygon(PlacePoints(MOLDING_BLADE_OUTLINE), WOOD, opacity)
    blade_center = PlacePoints([(69.0, 19.8), (69.0, 35.5)])
    canvas.DrawPolyline(blade_center, PlaceWidth(1.1), WOOD_HIGHLIGHT, opacity * 0.55)
    canvas.DrawPolyline(
        PlacePoints(MOLDING_TAIL_HIGHLIGHT),
        PlaceWidth(0.5),
        WOOD_HIGHLIGHT,
        opacity * 0.7,
    )

    crown_highlight = ScalePointsTowardCentroid(FELT_OUTLINE[0:5], FELT_CENTROID, 0.97)
    canvas.DrawPolyline(
        PlacePoints(crown_highlight),
        PlaceWidth(0.6),
        WARM_IVORY,
        opacity * 0.7,
    )


def DrawHammerScene(canvas, time_seconds):
    opacity = HammerOpacity(time_seconds)
    if opacity <= 0.0:
        return
    DrawHammer(
        canvas,
        HammerAngleDegrees(time_seconds),
        opacity,
        ImpactSquashAmount(time_seconds),
    )


def SignalBaselineY(time_seconds):
    progress = SmoothStep(PhaseProgress(time_seconds, 1.00, 1.28))
    return 10.0 + 25.0 * progress


def ScrollOffsetX(time_seconds):
    return -27.0 * SmoothStep(PhaseProgress(time_seconds, 1.15, 1.95))


def DrawSignal(canvas, time_seconds):
    baseline_y = SignalBaselineY(time_seconds)
    if time_seconds < 0.64:
        canvas.DrawLine(0.0, baseline_y, DISPLAY_WIDTH, baseline_y, 0.5, STRING_IVORY, 0.58)
        return

    scroll_offset_x = ScrollOffsetX(time_seconds)
    narrative_fade = 1.0 - SmoothStep(PhaseProgress(time_seconds, 3.05, 3.25))
    signal_progress = PhaseProgress(time_seconds, 0.64, 1.95)
    waveform_fade = (
        1.0
        - SmoothStep(PhaseProgress(time_seconds, 1.62, 2.08))
    ) * narrative_fade
    wave_center_x = 69.0 + 38.0 * EaseOutCubic(signal_progress)
    wave_amplitude = (
        6.0
        * SmoothStep(PhaseProgress(time_seconds, 0.64, 0.73))
        * (1.0 - 0.42 * signal_progress)
    )
    points = []
    for screen_index in range(161):
        screen_x = float(screen_index)
        world_x = screen_x - scroll_offset_x
        distance = world_x - wave_center_x
        envelope = math.exp(-0.0019 * distance * distance)
        point_y = baseline_y + wave_amplitude * envelope * math.sin(distance * 0.82)
        points.append((screen_x, point_y))
    canvas.DrawPolyline(points, 0.7, WARM_IVORY, waveform_fade)

    digital_progress = SmoothStep(PhaseProgress(time_seconds, 1.18, 1.96))
    digital_fade = (
        1.0
        - SmoothStep(PhaseProgress(time_seconds, 3.02, 3.22))
    ) * narrative_fade
    playhead_progress = PhaseProgress(
        time_seconds,
        PLAYHEAD_SWEEP_START_SECONDS,
        PLAYHEAD_SWEEP_END_SECONDS,
    )
    playhead_world_x = 45.0 + 117.0 * playhead_progress
    playhead_active = 0.0 < playhead_progress < 1.0
    block_positions = [
        (57.0, -1.4, 5.5),
        (69.0, 1.4, 6.0),
        (82.0, -0.8, 4.5),
        (94.0, 1.0, 7.0),
        (108.0, -1.1, 5.5),
        (121.0, 0.8, 6.0),
        (135.0, -0.7, 5.0),
        (147.0, 0.9, 3.5),
    ]
    visible_block_count = math.ceil(len(block_positions) * digital_progress)
    for block_index, (block_x, vertical_offset, block_width) in enumerate(block_positions):
        if block_index >= visible_block_count:
            break
        if block_index in (2, 5):
            block_color = BRIGHT_IVORY
            block_opacity = digital_fade
        else:
            block_color = WARM_IVORY
            block_opacity = (0.58 + 0.42 * digital_progress) * digital_fade
        block_pulse = 0.0
        if playhead_active:
            playhead_distance = block_x + block_width / 2.0 - playhead_world_x
            block_pulse = math.exp(-(playhead_distance * playhead_distance) / 72.0)
        block_opacity = block_opacity + (digital_fade - block_opacity) * block_pulse
        block_lift = 1.2 * block_pulse
        canvas.DrawLine(
            block_x + scroll_offset_x,
            baseline_y + vertical_offset - block_lift,
            block_x + block_width + scroll_offset_x,
            baseline_y + vertical_offset - block_lift,
            1.4,
            block_color,
            block_opacity,
        )

def DrawImpact(canvas, time_seconds):
    flash_progress = PhaseProgress(time_seconds, 0.635, 0.765)
    if flash_progress <= 0.0 or flash_progress >= 1.0:
        return
    opacity = math.sin(flash_progress * math.pi)
    canvas.DrawDisc(69.0, 10.0, 6.6, BRIGHT_IVORY, opacity * 0.06)
    canvas.DrawDisc(69.0, 10.0, 3.0, WARM_IVORY, opacity * 0.16)
    canvas.DrawDisc(69.0, 10.0, 0.95, WARM_IVORY, opacity)
    particle_vectors = [
        (-6.0, -4.2),
        (-2.0, -6.0),
        (3.3, -5.1),
        (6.2, -2.7),
        (5.5, 2.6),
    ]
    for particle_index, (direction_x, direction_y) in enumerate(particle_vectors):
        travel = EaseOutCubic(flash_progress) * 1.25
        particle_x = 69.0 + direction_x * travel
        particle_y = 10.0 + direction_y * travel + 2.2 * travel * travel
        radius = 0.6 if particle_index < 3 else 0.44
        canvas.DrawDisc(
            particle_x,
            particle_y,
            radius,
            BRIGHT_IVORY,
            opacity * (1.0 - 0.11 * particle_index),
        )


def DrawNoteHead(canvas, center_x, center_y, scale, growth, opacity):
    if growth <= 0.0:
        return
    canvas.DrawEllipse(
        center_x,
        center_y,
        2.0 * scale * growth,
        1.5 * scale * growth,
        NOTE_HEAD_TILT_RADIANS,
        WARM_IVORY,
        opacity,
    )


def DrawEighthNote(canvas, head_x, head_y, scale, progress, opacity):
    growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0))
    DrawNoteHead(canvas, head_x, head_y, scale, growth, opacity)
    stem_x = head_x + 1.6 * scale
    stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0))
    stem_rise = 9.2 * scale * stem_progress
    if stem_rise > 0.5:
        canvas.DrawLine(
            stem_x,
            head_y - 0.3 * scale,
            stem_x,
            head_y - stem_rise,
            max(0.6, 0.55 * scale),
            WARM_IVORY,
            opacity,
        )
    flag_progress = PhaseProgress(progress, 0.6, 1.0)
    if flag_progress > 0.0:
        stem_top = head_y - 9.2 * scale
        flag_points = BezierPoints(
            (stem_x, stem_top),
            (stem_x + 3.4 * scale, stem_top + 1.3 * scale),
            (stem_x + 3.9 * scale, stem_top + 3.6 * scale),
            (stem_x + 1.7 * scale, stem_top + 5.6 * scale),
            segment_count=14,
        )
        canvas.DrawPartialPolyline(
            flag_points,
            EaseOutCubic(flag_progress),
            max(0.6, 0.5 * scale),
            WARM_IVORY,
            opacity,
        )


def DrawQuarterNoteStemDown(canvas, head_x, head_y, scale, progress, opacity):
    growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0))
    DrawNoteHead(canvas, head_x, head_y, scale, growth, opacity)
    stem_x = head_x - 1.6 * scale
    stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0))
    stem_drop = 9.6 * scale * stem_progress
    if stem_drop > 0.5:
        canvas.DrawLine(
            stem_x,
            head_y + 0.3 * scale,
            stem_x,
            head_y + stem_drop,
            max(0.6, 0.55 * scale),
            WARM_IVORY,
            opacity,
        )


def DrawBeamedEighthPair(canvas, head_x, head_y, scale, progress, opacity):
    second_x = head_x + 5.6 * scale
    second_y = head_y - 1.5 * scale
    first_growth = SmoothStep(Clamp(progress * 2.4, 0.0, 1.0))
    second_growth = SmoothStep(Clamp((progress - 0.08) * 2.4, 0.0, 1.0))
    DrawNoteHead(canvas, head_x, head_y, scale, first_growth, opacity)
    DrawNoteHead(canvas, second_x, second_y, scale, second_growth, opacity)
    stem_height = 8.8 * scale
    first_stem_x = head_x + 1.6 * scale
    second_stem_x = second_x + 1.6 * scale
    first_stem_progress = SmoothStep(Clamp(progress * 1.7, 0.0, 1.0))
    second_stem_progress = SmoothStep(Clamp((progress - 0.08) * 1.7, 0.0, 1.0))
    if first_stem_progress > 0.05:
        canvas.DrawLine(
            first_stem_x,
            head_y - 0.3 * scale,
            first_stem_x,
            head_y - stem_height * first_stem_progress,
            max(0.6, 0.55 * scale),
            WARM_IVORY,
            opacity,
        )
    if second_stem_progress > 0.05:
        canvas.DrawLine(
            second_stem_x,
            second_y - 0.3 * scale,
            second_stem_x,
            second_y - stem_height * second_stem_progress,
            max(0.6, 0.55 * scale),
            WARM_IVORY,
            opacity,
        )
    beam_progress = EaseOutCubic(PhaseProgress(progress, 0.6, 1.0))
    if beam_progress > 0.0:
        beam_start = (first_stem_x, head_y - stem_height)
        beam_end = (second_stem_x, second_y - stem_height)
        beam_tip_x = beam_start[0] + (beam_end[0] - beam_start[0]) * beam_progress
        beam_tip_y = beam_start[1] + (beam_end[1] - beam_start[1]) * beam_progress
        canvas.DrawLine(
            beam_start[0],
            beam_start[1],
            beam_tip_x,
            beam_tip_y,
            max(0.8, 1.05 * scale),
            WARM_IVORY,
            opacity,
        )


PLAYHEAD_SWEEP_START_SECONDS = 2.10
PLAYHEAD_SWEEP_END_SECONDS = 3.00

EMERGING_NOTE_SPECS = [
    ("eighth", 82.0, 1.60, 0.95, 8.0, 0.0),
    ("quarter", 108.0, 1.78, 1.18, 13.0, 2.1),
    ("beamed", 131.0, 1.96, 0.78, 5.5, 4.2),
]
NOTE_EMERGENCE_DURATION_SECONDS = 0.45


def DrawEmergingNotes(canvas, time_seconds):
    narrative_opacity = 1.0 - SmoothStep(PhaseProgress(time_seconds, 3.05, 3.25))
    if narrative_opacity <= 0.0:
        return
    baseline_y = 35.0
    scroll_offset_x = ScrollOffsetX(time_seconds)
    for kind, note_x, start_seconds, scale, rise, sway_phase in EMERGING_NOTE_SPECS:
        progress = PhaseProgress(
            time_seconds,
            start_seconds,
            start_seconds + NOTE_EMERGENCE_DURATION_SECONDS,
        )
        if progress <= 0.0:
            continue
        settled = SmoothStep(progress)
        float_drift = 2.5 * Clamp(time_seconds - start_seconds, 0.0, 1.0)
        bob = 0.7 * math.sin(5.0 * time_seconds + sway_phase) * settled
        playhead_progress = PhaseProgress(
            time_seconds,
            PLAYHEAD_SWEEP_START_SECONDS,
            PLAYHEAD_SWEEP_END_SECONDS,
        )
        note_pop = 0.0
        if 0.0 < playhead_progress < 1.0:
            playhead_distance = note_x - (45.0 + 117.0 * playhead_progress)
            note_pop = math.exp(-(playhead_distance * playhead_distance) / 72.0)
        head_y = (
            baseline_y
            - 3.0
            - rise * EaseOutCubic(progress)
            - float_drift
            + bob
            - 1.0 * note_pop
        )
        sway = 1.1 * math.sin(2.4 * time_seconds + sway_phase) * settled
        screen_x = note_x + scroll_offset_x + sway
        popped_scale = scale * (1.0 + 0.15 * note_pop)
        opacity = narrative_opacity * SmoothStep(Clamp(progress * 3.0, 0.0, 1.0))
        if kind == "eighth":
            DrawEighthNote(canvas, screen_x, head_y, popped_scale, progress, opacity)
        elif kind == "quarter":
            DrawQuarterNoteStemDown(
                canvas, screen_x, head_y, popped_scale, progress, opacity
            )
        else:
            DrawBeamedEighthPair(
                canvas, screen_x, head_y, popped_scale, progress, opacity
            )


def _BuildWordmarkGlyphs():
    d_bowl = BezierPoints((0.0, 0.0), (4.4, 0.2), (4.4, 6.8), (0.0, 7.0), 12)
    s_curve = [
        (3.3, 0.95),
        (2.7, 0.25),
        (1.6, 0.05),
        (0.7, 0.45),
        (0.25, 1.25),
        (0.35, 2.15),
        (1.0, 2.9),
        (2.3, 3.55),
        (3.3, 4.25),
        (3.6, 5.15),
        (3.45, 6.05),
        (2.8, 6.7),
        (1.7, 6.95),
        (0.7, 6.65),
        (0.2, 5.9),
    ]
    return {
        "M": (4.6, [[(0.0, 7.0), (0.0, 0.0), (2.3, 4.3), (4.6, 0.0), (4.6, 7.0)]]),
        "I": (1.0, [[(0.5, 0.0), (0.5, 7.0)]]),
        "D": (3.5, [[(0.0, 0.0), (0.0, 7.0)], d_bowl]),
        "S": (3.7, [s_curve]),
        "T": (4.0, [[(0.0, 0.0), (4.0, 0.0)], [(2.0, 0.0), (2.0, 7.0)]]),
        "H": (4.2, [[(0.0, 0.0), (0.0, 7.0)], [(4.2, 0.0), (4.2, 7.0)], [(0.0, 3.6), (4.2, 3.6)]]),
        " ": (2.4, []),
    }


WORDMARK_GLYPHS = _BuildWordmarkGlyphs()
WORDMARK_TEXT = "MIDI SMITH"
WORDMARK_SCALE = 2.25
WORDMARK_LETTER_SPACING_UNITS = 1.5
WORDMARK_ORIGIN_X = 45.5
WORDMARK_ORIGIN_Y = 30.0
WORDMARK_STROKE_WIDTH = 0.62 * WORDMARK_SCALE
WORDMARK_REVEAL_START_SECONDS = 3.51
WORDMARK_LETTER_STAGGER_SECONDS = 0.05
WORDMARK_LETTER_FADE_SECONDS = 0.12
WORDMARK_SHINE_START_SECONDS = 4.20
WORDMARK_SHINE_END_SECONDS = 4.55

FINAL_NOTE_HEAD_CENTER = (25.0, 51.0)
FINAL_NOTE_SCALE = 2.7

RULE_Y = 51.0
RULE_SWEEP_START_SECONDS = 3.93
RULE_SWEEP_END_SECONDS = 4.19
RULE_TIP_FADE_START_SECONDS = 4.19
RULE_TIP_FADE_END_SECONDS = 4.33


def MeasureWordmarkWidth():
    advance_sum = sum(WORDMARK_GLYPHS[character][0] for character in WORDMARK_TEXT)
    spacing_sum = WORDMARK_LETTER_SPACING_UNITS * (len(WORDMARK_TEXT) - 1)
    return (advance_sum + spacing_sum) * WORDMARK_SCALE


def DrawFinalNote(canvas, time_seconds):
    head_progress = SmoothStep(PhaseProgress(time_seconds, 3.33, 3.49))
    if head_progress <= 0.0:
        return
    stem_progress = SmoothStep(PhaseProgress(time_seconds, 3.37, 3.57))
    flag_progress = PhaseProgress(time_seconds, 3.53, 3.75)
    scale = FINAL_NOTE_SCALE
    head_x, head_y = FINAL_NOTE_HEAD_CENTER
    canvas.DrawEllipse(
        head_x,
        head_y,
        2.0 * scale * head_progress,
        1.5 * scale * head_progress,
        NOTE_HEAD_TILT_RADIANS,
        WARM_IVORY,
        Clamp(head_progress * 1.6, 0.0, 1.0),
    )
    stem_x = head_x + 1.6 * scale
    stem_bottom = head_y - 0.3 * scale
    stem_top_full = head_y - 11.3 * scale
    if stem_progress > 0.0:
        stem_top = stem_bottom - (stem_bottom - stem_top_full) * stem_progress
        canvas.DrawLine(stem_x, stem_bottom, stem_x, stem_top, 1.8, WARM_IVORY, 1.0)
    if flag_progress > 0.0:
        outer_points = BezierPoints(
            (stem_x, stem_top_full),
            (stem_x + 4.5 * scale, stem_top_full + 1.6 * scale),
            (stem_x + 5.0 * scale, stem_top_full + 4.6 * scale),
            (stem_x + 2.1 * scale, stem_top_full + 7.2 * scale),
            segment_count=16,
        )
        inner_points = BezierPoints(
            (stem_x, stem_top_full + 1.0 * scale),
            (stem_x + 3.4 * scale, stem_top_full + 2.2 * scale),
            (stem_x + 3.9 * scale, stem_top_full + 4.4 * scale),
            (stem_x + 1.9 * scale, stem_top_full + 6.9 * scale),
            segment_count=16,
        )
        revealed_count = max(2, round(EaseOutCubic(flag_progress) * 16) + 1)
        flag_polygon = outer_points[:revealed_count] + inner_points[:revealed_count][::-1]
        canvas.FillPolygon(flag_polygon, WARM_IVORY, 1.0)


def DrawWordmark(canvas, time_seconds):
    shine_progress = PhaseProgress(
        time_seconds,
        WORDMARK_SHINE_START_SECONDS,
        WORDMARK_SHINE_END_SECONDS,
    )
    shine_x = 40.0 + 110.0 * shine_progress
    shine_active = 0.0 < shine_progress < 1.0
    current_x = WORDMARK_ORIGIN_X
    drawn_glyph_index = 0
    for character in WORDMARK_TEXT:
        advance, strokes = WORDMARK_GLYPHS[character]
        if strokes:
            reveal_start = (
                WORDMARK_REVEAL_START_SECONDS
                + WORDMARK_LETTER_STAGGER_SECONDS * drawn_glyph_index
            )
            reveal = SmoothStep(
                PhaseProgress(
                    time_seconds,
                    reveal_start,
                    reveal_start + WORDMARK_LETTER_FADE_SECONDS,
                )
            )
            if reveal > 0.0:
                settle_offset = 1.4 * (1.0 - EaseOutCubic(reveal))
                shine_weight = 0.0
                if shine_active:
                    glyph_center_x = current_x + advance * WORDMARK_SCALE / 2.0
                    shine_distance = glyph_center_x - shine_x
                    shine_weight = math.exp(-(shine_distance * shine_distance) / 50.0)
                for stroke in strokes:
                    placed = [
                        (
                            current_x + point_x * WORDMARK_SCALE,
                            WORDMARK_ORIGIN_Y + settle_offset + point_y * WORDMARK_SCALE,
                        )
                        for point_x, point_y in stroke
                    ]
                    canvas.DrawPolyline(
                        placed,
                        WORDMARK_STROKE_WIDTH,
                        WARM_IVORY,
                        reveal,
                    )
                    if shine_weight > 0.02:
                        canvas.DrawPolyline(
                            placed,
                            WORDMARK_STROKE_WIDTH * 1.7,
                            BRIGHT_IVORY,
                            reveal * 0.75 * shine_weight,
                        )
            drawn_glyph_index += 1
        current_x += (advance + WORDMARK_LETTER_SPACING_UNITS) * WORDMARK_SCALE


def DrawBaselineRule(canvas, time_seconds):
    sweep = EaseOutCubic(
        PhaseProgress(time_seconds, RULE_SWEEP_START_SECONDS, RULE_SWEEP_END_SECONDS)
    )
    if sweep <= 0.0:
        return
    rule_start_x = WORDMARK_ORIGIN_X
    rule_end_x = WORDMARK_ORIGIN_X + MeasureWordmarkWidth()
    tip_x = rule_start_x + (rule_end_x - rule_start_x) * sweep
    canvas.DrawLine(rule_start_x, RULE_Y, tip_x, RULE_Y, 0.55, STRING_IVORY, 0.6)
    tip_fade = 1.0 - SmoothStep(
        PhaseProgress(time_seconds, RULE_TIP_FADE_START_SECONDS, RULE_TIP_FADE_END_SECONDS)
    )
    if tip_fade > 0.0:
        canvas.DrawDisc(tip_x, RULE_Y, 2.4, BRIGHT_IVORY, 0.10 * tip_fade)
        canvas.DrawDisc(tip_x, RULE_Y, 0.85, BRIGHT_IVORY, 0.85 * tip_fade)


def DrawFinalIdentity(canvas, time_seconds):
    DrawFinalNote(canvas, time_seconds)
    DrawWordmark(canvas, time_seconds)
    DrawBaselineRule(canvas, time_seconds)


def RenderFrame(time_seconds):
    canvas = Canvas(DISPLAY_WIDTH, DISPLAY_HEIGHT)
    DrawSignal(canvas, time_seconds)
    DrawHammerScene(canvas, time_seconds)
    DrawImpact(canvas, time_seconds)
    DrawEmergingNotes(canvas, time_seconds)
    DrawFinalIdentity(canvas, time_seconds)
    return canvas.Downsample()


def WritePpm(path, width, height, pixels):
    with path.open("wb") as output:
        output.write(f"P6\n{width} {height}\n255\n".encode("ascii"))
        output.write(pixels)


def ComposeStoryboard(frames, column_count):
    gutter_size = 2
    row_count = math.ceil(len(frames) / column_count)
    storyboard_width = (
        DISPLAY_WIDTH * column_count + gutter_size * (column_count - 1)
    )
    storyboard_height = DISPLAY_HEIGHT * row_count + gutter_size * (row_count - 1)
    output = bytearray(storyboard_width * storyboard_height * 3)
    for frame_index, frame in enumerate(frames):
        origin_x = (frame_index % column_count) * (DISPLAY_WIDTH + gutter_size)
        origin_y = (frame_index // column_count) * (DISPLAY_HEIGHT + gutter_size)
        for frame_y in range(DISPLAY_HEIGHT):
            source_offset = frame_y * DISPLAY_WIDTH * 3
            destination_offset = (
                (origin_y + frame_y) * storyboard_width + origin_x
            ) * 3
            output[destination_offset : destination_offset + DISPLAY_WIDTH * 3] = frame[
                source_offset : source_offset + DISPLAY_WIDTH * 3
            ]
    return storyboard_width, storyboard_height, bytes(output)


def RunFfmpeg(arguments):
    executable = shutil.which("ffmpeg")
    if executable is None:
        raise RuntimeError("ffmpeg is required to export splash previews")
    subprocess.run(
        [executable, "-hide_banner", "-loglevel", "error", "-y", *arguments],
        check=True,
    )


def ExportPreviews(output_directory):
    output_directory.mkdir(parents=True, exist_ok=True)
    frame_count = round(ANIMATION_DURATION_SECONDS * FRAME_RATE_HZ)
    frame_times = [
        frame_index / FRAME_RATE_HZ for frame_index in range(frame_count)
    ]
    frames = [RenderFrame(frame_time) for frame_time in frame_times]

    with tempfile.TemporaryDirectory(prefix="midi-smith-splash-") as temporary_directory:
        temporary_path = Path(temporary_directory)
        for frame_index, frame in enumerate(frames):
            WritePpm(
                temporary_path / f"frame_{frame_index:03d}.ppm",
                DISPLAY_WIDTH,
                DISPLAY_HEIGHT,
                frame,
            )

        native_gif_path = (
            output_directory / f"midi_smith_splash_{SPLASH_VERSION}_160x80.gif"
        )
        preview_gif_path = (
            output_directory / f"midi_smith_splash_{SPLASH_VERSION}_preview.gif"
        )
        preview_mp4_path = (
            output_directory / f"midi_smith_splash_{SPLASH_VERSION}_preview.mp4"
        )
        RunFfmpeg(
            [
                "-framerate",
                str(FRAME_RATE_HZ),
                "-i",
                str(temporary_path / "frame_%03d.ppm"),
                "-filter_complex",
                "split[a][b];[a]palettegen=max_colors=256:stats_mode=diff[p];"
                "[b][p]paletteuse=dither=bayer:bayer_scale=3",
                str(native_gif_path),
            ]
        )
        RunFfmpeg(
            [
                "-framerate",
                str(FRAME_RATE_HZ),
                "-i",
                str(temporary_path / "frame_%03d.ppm"),
                "-filter_complex",
                "scale=640:320:flags=neighbor,split[a][b];"
                "[a]palettegen=max_colors=256:stats_mode=diff[p];"
                "[b][p]paletteuse=dither=bayer:bayer_scale=3",
                str(preview_gif_path),
            ]
        )
        RunFfmpeg(
            [
                "-framerate",
                str(FRAME_RATE_HZ),
                "-i",
                str(temporary_path / "frame_%03d.ppm"),
                "-vf",
                "scale=1280:640:flags=neighbor",
                "-c:v",
                "libx264",
                "-pix_fmt",
                "yuv420p",
                "-movflags",
                "+faststart",
                str(preview_mp4_path),
            ]
        )

        keyframe_times = [0.42, 0.66, 1.32, 2.20, 2.60, 4.40]
        storyboard_frames = [RenderFrame(keyframe_time) for keyframe_time in keyframe_times]
        storyboard_width, storyboard_height, storyboard = ComposeStoryboard(
            storyboard_frames,
            3,
        )
        storyboard_ppm_path = temporary_path / "storyboard.ppm"
        WritePpm(
            storyboard_ppm_path,
            storyboard_width,
            storyboard_height,
            storyboard,
        )
        storyboard_path = (
            output_directory / f"midi_smith_splash_{SPLASH_VERSION}_storyboard.png"
        )
        RunFfmpeg(
            [
                "-i",
                str(storyboard_ppm_path),
                "-vf",
                f"scale={storyboard_width * 4}:{storyboard_height * 4}:flags=neighbor",
                str(storyboard_path),
            ]
        )

    return [
        native_gif_path,
        preview_gif_path,
        preview_mp4_path,
        storyboard_path,
    ]


GOLDEN_FRAME_TIMES_SECONDS = [0.42, 0.66, 1.32, 1.92, 2.60, 3.70, 4.40]


def ExportGoldenFrames(output_directory):
    output_directory.mkdir(parents=True, exist_ok=True)
    exported_paths = []
    for frame_time in GOLDEN_FRAME_TIMES_SECONDS:
        frame = RenderFrame(frame_time)
        milliseconds = round(frame_time * 1000)
        frame_path = output_directory / f"frame_{milliseconds:04d}ms.ppm"
        WritePpm(frame_path, DISPLAY_WIDTH, DISPLAY_HEIGHT, frame)
        exported_paths.append(frame_path)
    return exported_paths


def ParseArguments():
    parser = argparse.ArgumentParser(
        description="Render the Midi Smith 160x80 splash-screen animatic."
    )
    parser.add_argument(
        "--output-directory",
        type=Path,
        default=Path("firmwares/main-board/docs/assets/splash"),
    )
    parser.add_argument(
        "--export-golden-directory",
        type=Path,
        default=None,
    )
    return parser.parse_args()


def Main():
    arguments = ParseArguments()
    if arguments.export_golden_directory is not None:
        generated_paths = ExportGoldenFrames(arguments.export_golden_directory)
    else:
        generated_paths = ExportPreviews(arguments.output_directory)
    for generated_path in generated_paths:
        print(generated_path)


if __name__ == "__main__":
    Main()
