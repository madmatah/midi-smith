## Theory of Operation: NoteOn Velocity Engine

**Responsibility:** Decision making, strike validation, and MIDI velocity latching.

### 1 MIDI Velocity Mapping (Goebl's Model)

The engine uses a logarithmic mapping based on the study by *Goebl & Bresin (2001)* to match human auditory perception (Weber-Fechner Law).

**The Formula:**
$\text{MIDI Velocity} = 57.96 + 71.3 \cdot \log_{10}(V_{hammer})$

* **Rationale:** A logarithmic curve ensures that the difference between *piano* and *pianissimo* is as expressive as the difference between *forte* and *fortissimo*.
* **Implementation:** The result is clamped between 1 and 127.

### 2 Key Thresholds (Reference Measurements)

The engine uses specific position thresholds (in normalized units) to drive the state machine:

* **Active Zone (e.g., 0.55):** The stage begins monitoring for a potential strike (Damper lift area).
* **Escapement / Let-off Point (e.g., 0.054):** The mechanical "point of no return." This is the critical measurement trigger.
* **Strike Point (e.g., 0.005):** The virtual string contact. Trigger for the actual Note-On.
* **Drop / Abort Point (e.g., 0.078):** The position where a hammer falls after a failed (silent) escapement.
* **Re-arm Threshold (e.g., 0.1):** Point where the logic allows a new strike detection.

### 3 State Machine & Latching Strategy

1. **Tracking Phase:** When the hammer enters the **Active Zone**, the engine monitors the `last_hammer_speed_m_per_s` (`ShankToHammerKinematicsStage` output).

2. **Snapshot (The Latch):** As soon as the position crosses the **Escapement Point** moving toward the string:
    * The engine captures the **hammer speed** at this exact moment.
    * This value is converted to MIDI velocity via the mapping formula.
    * The value is stored as `LatchedMidiVelocity`.
    * *Rationale:* After the escapement point, the hammer is in "free flight." Subsequent velocity changes are due to friction or gravity, not player intent.

3. **Validation (The Commit):**
    * **Success:** If the hammer reaches the **Strike Point**: A `Note-On` is triggered using the `LatchedMidiVelocity`.
    * **Abort:** If the hammer reverses direction and reaches the **Drop Point** without touching the Strike Point: The strike is aborted (Silent Press).


### 5 Double Strike & Repetition

To support rapid repetitions (trills):
* The engine **re-arms** as soon as the hammer velocity becomes positive (returning toward rest) and the position exceeds a **Re-arm Threshold**.
* This allows a second strike to be captured even if the hammer hasn't returned to the rest position or the full Active Zone, enabling authentic grand piano repetition behavior.

