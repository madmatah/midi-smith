## Theory of Operation: MIDI Velocity Engine

**Responsibility:** Decision making, strike validation, kinematic conversion, and MIDI velocity latching.

### 1 Kinematics: From Sensor to Hammer Head

Before calculating a MIDI value, the engine must convert the relative speed measured by the sensor into the physical speed of the hammer head.

#### 1.1 Linearization Scale ($\Delta d$):

The sensor measures a normalized range $[0.0, 1.0]$.

* $D_{rest}$: distance between sensor and hammer shank at rest
* $D_{strike}$: distance between sensor and hammer shank when striking the string
* $\Delta d$ : otal physical travel at sensor point

$\Delta d = D_{rest} - D_{strike} = 1.0\text{ unit}$

Example:
* $D_{rest} = 8.0\text{mm}$
* $D_{strike} = 1.9\text{mm}$
* $\Delta d = 6.1\text{ mm} = 1.0\text{ unit}$


#### 1.2 Leverage Ratio ($L_{ratio}$):

The sensor is placed at $R_{sensor}$ (e.g. 15 mm) from the pivot, while the hammer head is at $R_{hammer}$ (typically $\approx 120\text{ mm}$).

$L_{ratio} = \frac{R_{hammer}}{R_{sensor}}$

The hammer head moves $L_{ratio}$ times faster than the shank at the sensor point.

#### 1.3 Final hammer speed conversion ($V_{m/s}$):

To obtain the hammer speed in meters per second (m/s) as required by academic models:

$V_{m/s} = V_{normalized/ms} \times 6.1 \times L_{ratio}$

*(Note: $mm/ms$ is equivalent to $m/s$, simplifying the unit conversion.)*

### 2 MIDI Velocity Mapping (Goebl's Model)

The engine uses a logarithmic mapping based on the study by *Goebl & Bresin (2001)* to match human auditory perception (Weber-Fechner Law).

**The Formula:**
$\text{MIDI Velocity} = 57.96 + 71.3 \cdot \log_{10}(V_{m/s})$

* **Rationale:** A logarithmic curve ensures that the difference between *piano* and *pianissimo* is as expressive as the difference between *forte* and *fortissimo*.
* **Implementation:** The result is clamped between 1 and 127.

### 3 Key Thresholds (Reference Measurements)

* **Active Zone (e.g. 0.10):** The stage begins monitoring for a potential strike.
* **Escapement Point (e.g. 0.02):** The mechanical "point of no return." Measurement trigger.
* **Strike Point (e.g. 0.005):** The virtual string contact. Trigger for MIDI Note-On.
* **Drop/Abort Point (e.g. 0.06):** The position where a hammer falls after a failed (silent) escapement.

### 4 State Machine & Latching Strategy

1. **Tracking Phase:** When the hammer enters the **Active Zone**, the engine monitors the `SlidingLinearRegression` output.

2. **Snapshot (The Latch):** As soon as the position crosses the **Escapement Point** moving toward the string:
    * The engine captures the **current instantaneous velocity**.
    * This value is converted to $V_{m/s}$ and then to MIDI via the `IVelocityMapper`.
    * The value is stored as `LatchedMidiVelocity`.
    * *Rationale:* After **Escapement point**, the hammer is in "free flight." Subsequent velocity changes are due to friction, not player intent.

3. **Validation (The Commit):**
    * **Success:** If the hammer reaches the **Strike Point**: A `MIDI Note-On` is triggered using the `LatchedMidiVelocity`.
    * **Abort:** If the hammer reverses direction and reaches the **Drop Point** without touching the Strike Point: The strike is aborted (Silent Press).


### 5 Double Strike & Repetition

To support rapid repetitions (trills):

To support rapid repetitions (trills):
* The engine **re-arms** as soon as the hammer velocity becomes positive (returning toward rest) and the position exceeds a **Re-arm Threshold**.
* This allows a second strike to be captured even if the hammer hasn't returned to the rest position (> **Active Zone**).

