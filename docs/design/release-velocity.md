# Theory of Operation: Note-Off & Damper Engine

**Responsibility:** Estimation of damper release speed from hammer shank motion, and MIDI Note-Off velocity mapping.

---

## Introduction: The Fundamental Constraint

In a grand piano action, the damper is mechanically coupled to the key, not to the hammer. Ideally, measuring the Note-Off velocity would require a dedicated sensor at the key or damper level to directly capture the speed at which the felt contacts the strings.

Since the current system only has optical sensors on the **hammer shanks**, the damper release speed cannot be measured directly and must be estimated from shank motion.

This constraint introduces a second, more fundamental problem: to avoid false Note-Off triggers during loud playing, the detection threshold must be placed **beyond the repetition catch point**. In a grand piano action, moving away from the string, the key mechanical positions are approximately:

- **Strike point** — hammer hits the string
- **Let-off** — escapement, hammer enters free flight
- **Drop point** — hammer resting on the repetition lever after a failed strike
- **Damper contact** — damper felt returns to the strings
- **Catch point** — repetition lever catches the hammer on return

The Note-Off threshold must therefore be set **above the catch point** to prevent a spurious Note-Off from being emitted while the key is still held down — for instance, when playing a forte note causes the hammer to engage the repetition catch mechanism while the finger remains depressed on the key. This means the detection point is physically earlier than the actual moment the damper touches the strings.

This inherent imprecision means that a highly accurate physical model of the damper velocity would not yield a meaningfully more realistic result. The system therefore favors a **pragmatic estimation approach**: compute a representative shank speed near the detection zone, and map it directly to MIDI velocity through a perceptually tuned curve.

---

## 1. Damper Release Speed Estimation

### 1.1 Design Rationale

Unlike the Note-On pipeline, which requires sub-millisecond reactivity to capture the hammer speed at escapement, the Note-Off pipeline operates under **relaxed latency constraints**. A few milliseconds of additional delay on a key release is imperceptible to the player and has no impact on musical accuracy.

This relaxed constraint allows the use of heavier low-pass filtering to suppress the noise present on the signal, which in turn enables a simpler and less computationally expensive differentiation algorithm. The trade-off is a slight attenuation of the velocity amplitude, which is compensated by a calibration scale factor (`SIGNAL_SMOOTHED_POSITION_SCALE_FACTOR`).

### 1.2 Pipeline Architecture (`DamperReleasePhysicalStage`)

The pipeline processes the normalized shank position through the following stages:

1. **Position Smoothing (`SmoothedPositionFilter`):** A Simple Moving Average with a large window is applied to the raw normalized position. This produces a stable, noise-free position signal, captured as `last_shank_position_smoothed_norm`. The large window introduces significant latency, which is acceptable given the relaxed timing requirements.

2. **Physical Scaling (`PhysicalPositionStage`):** The smoothed normalized position is scaled to millimeters using $\Delta d$ (the physical travel of the shank at the sensor point), as described in the *Physical Position Stage* document.

3. **Speed Estimation (`SmartFallingShankSpeedEstimator`):** Speed is computed only when a Note-On is currently active and the shank is within the active zone (above `HAMMER_POSITION_DAMPER`), avoiding unnecessary computation while the key is at rest. When active, the speed is estimated by chaining a **Central Difference** differentiator with a **Simple Moving Average** (`SIGNAL_FALLING_SHANK_SPEED_SMA_WINDOW_SIZE`). Otherwise, the output is held at $0.0\ \text{m/s}$.

4. **Time Normalization (`TicksToMsNormalizer`):** The raw slope (in $mm/tick$) is converted to $m/s$.

5. **Amplitude Correction (`LinearScaler<SIGNAL_SMOOTHED_POSITION_SCALE_FACTOR>`):** The heavy filtering attenuates the peak velocity amplitude. A fixed scale factor compensates for this attenuation, restoring a representative value. The result is captured as `last_shank_falling_speed_m_per_s`.

### 1.3 Note on Differentiation Algorithm Choice

The Note-On pipeline uses a `SlidingLinearRegression` (OLS over $N$ samples) for speed estimation, offering superior noise rejection at low latency. For the Note-Off pipeline, the simpler `CentralDifference` is used instead. Given the heavier SMA applied beforehand and the relaxed latency budget, the Central Difference provides sufficient accuracy at a meaningfully lower computational cost.

---

## 2. MIDI Note-Off Velocity Mapping (`NoteReleaseDetectorStage`)

### 2.1 Velocity Mapper

The estimated shank speed in $m/s$ is mapped to a MIDI velocity (1–127) using a **logarithmic curve** (`LogarithmicVelocityMapper`), parameterized by:

- `NOTE_OFF_VELOCITY_MAX_M_PER_S`: The reference speed corresponding to MIDI velocity 127.
- `NOTE_OFF_VELOCITY_SHAPE_FACTOR`: Controls the curvature of the logarithmic response.

The logarithmic model ensures that the perceptual difference between a slow and medium release is as expressive as the difference between a medium and fast release.

### 2.2 Detection Threshold

The Note-Off is triggered when `last_shank_position_smoothed_norm` crosses `HAMMER_POSITION_DAMPER` while moving toward rest. Using the smoothed position at this stage — rather than the raw position — provides inherent immunity to sensor noise near the threshold, and also improves temporal correlation with the speed signal, which is itself derived from the same smoothed position.

---

## 3. Thresholds & Detection Logic

- **Release Threshold (`HAMMER_POSITION_DAMPER`):** The normalized shank position above which the damper is considered to be in contact with the strings. As described in the introduction, this threshold is intentionally set beyond the catch point to avoid false Note-Off triggers during forte playing.
- **Hysteresis:** A small buffer zone around the threshold prevents Note-Off chattering if the key hovers near the detection point.
- **Smoothing:** Detection uses `last_shank_position_smoothed_norm` rather than the raw position, providing inherent noise immunity at the cost of a small additional latency.

---

## 4. State Machine & Latching Strategy

1. **Monitoring phase:** While `is_note_on` is true, the engine continuously tracks `last_shank_position_smoothed_norm`.

2. **The latch:** As long as the smoothed position remains above `HAMMER_POSITION_DAMPER` (key depressed, damper lifted), the engine continuously updates `latched_release_speed` with the latest value of `last_shank_falling_speed_m_per_s`.

3. **The trigger:** When `last_shank_position_smoothed_norm` crosses `HAMMER_POSITION_DAMPER` moving toward rest:
   - The engine captures the **last valid latched release speed**.
   - This value is passed to the `NoteOffVelocityMapper` to produce a MIDI velocity.
   - `OnNoteOff` is emitted and `is_note_on` is set to false.

---

## 5. Alternative Approaches

The following approaches were considered but not retained for the reasons described below.

### 5.1 Dedicated Key or Damper Sensors

The most direct solution would be to place optical sensors on the keys or on the damper mechanism itself. This would provide a direct measurement of damper speed, eliminating all estimation and the threshold placement problem described in the introduction.

This approach was not retained due to the significant additional hardware cost and mechanical complexity it would introduce — 22 additional sensors per board, along with the associated wiring and calibration overhead.

### 5.2 Analytical Kinematic Model

A full physical model of the piano action lever chain could convert the shank speed into a key edge speed ($V_{key}$, in $m/s$ at the fingertip). The global leverage ratio $K_{damper}$ is defined by the geometry of each lever stage:

$$K_{damper} = \left( \frac{R_{key\_edge}}{R_{key\_mech}} \right) \times \left( \frac{R_{wippen\_out}}{R_{wippen\_in}} \right) \times \left( \frac{R_{jack}}{R_{sensor}} \right)$$

Where:
- $R_{sensor}$: Distance from hammer shank pivot to the CNY70 sensor center.
- $R_{jack}$: Distance from hammer shank pivot to the jack contact point.
- $R_{wippen\_out} / R_{wippen\_in}$: Leverage ratio of the wippen (intermediate lever).
- $R_{key\_mech}$: Distance from the key balance rail to the wippen contact point.
- $R_{key\_edge}$: Distance from the key balance rail to the front edge of the key.

The resulting key edge velocity would be:

$$V_{rel} = V_{normalized/ms} \times \Delta d \times K_{damper}$$

This approach was not retained because the fundamental imprecision of the detection threshold (see Introduction) makes a rigorous kinematic model unnecessary. It would also require measuring the individual lever arm dimensions of each piano.

### 5.3 Empirical Lever Ratio Calibration

As an alternative to the analytical model, $K_{damper}$ can be determined empirically through a simple measurement protocol, bypassing the need to measure individual lever arm dimensions:

1. **Rest state:** Record the sensor position ($DR_{rest}$) and the key edge height ($H_{rest}$).
2. **Pressed state:** Depress the key by a known distance ($\Delta H$, typically a few millimeters, before escapement) and record the new sensor position ($DR_{pressed}$).

$$K_{empirical} = \frac{\Delta H}{DR_{pressed} - DR_{rest}}$$

The velocity conversion then simplifies to:

$$V_{m/s} = V_{normalized/ms} \times K_{empirical}$$

This method naturally accounts for real-world factors such as felt compression and sensor placement tolerances. It was not retained for the same reason as the analytical model: the detection threshold imprecision makes the additional per-piano calibration step unjustified.