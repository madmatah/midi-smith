## Theory of Operation: Shank Speed Estimation

**Responsibility:** Compute real-time physical shank **speed** ($m/s$) from absolute shank position ($mm$).

### Overview

The `SmartShankSlopeEstimator` is the DSP stage responsible for calculating the instantaneous velocity of the hammer shank. It operates within a dedicated pipeline that converts raw normalized data into physical units ($m/s$) before scaling it to the final hammer kinematics.

### The Gated Pipeline Architecture

To ensure signal integrity and focus processing power on the relevant mechanical phase, the system uses an **Active Zone Logic**. The speed is only computed when the shank enters the "Active Zone" (roughly corresponding to the damper lift point).

* **Input Gate:** A logical switch (`IsShankInActiveZone`) monitors the normalized shank position.
* **Idle State:** While the hammer is in the rest zone (Gate Closed), the output is forced to a constant $0.0 \text{ m/s}$.
* **Active State:** When the gate opens, the signal is routed through the `GuardedShankSpeedEstimator`.

### Dual-Stage Filtering Chain

To mitigate sensor noise and quantization jitter from the ADC (which are amplified by the non-linear linearization stage), the system employs two distinct filtering stages in series:

| Stage | Algorithm | Role |
| :--- | :--- | :--- |
| **Stage 1** | **Sliding Linear Regression** ($N=5$) | Extracts the slope ($v = \frac{dx}{dt}$) from 5 position samples using Ordinary Least Squares (OLS). It is extremely reactive with a window of only $0.27 \text{ ms}$ at $18 \text{ kHz}$. |
| **Stage 2** | **Simple Moving Average** ($N=16$) | Smooths the quantization noise and high-frequency jitter produced by the differentiator. It provides a stable "plateau" for velocity measurement. |


### Technical Specifications

* **Sampling Rate ($f_s$):** $18,000 \text{ Hz}$ ($\Delta t \approx 55 \mu\text{s}$ per sample).
* **Differentiator Window:** 5 samples ($\approx 0.27 \text{ ms}$).
* **Smoother Window:** 16 samples ($\approx 0.88 \text{ ms}$).
* **Total Group Delay:** $\approx 9.5 \text{ samples}$ ($\approx 0.52 \text{ ms}$). This delay is constant.
* **Units:**
    * **Input:** Absolute Shank Position in $mm$.
    * **Intermediate:** Speed in $mm/tick$.
    * **Output:** Physical Shank Velocity in $m/s$.


### Logic Flow Summary

1.  **Gate Check:** Monitor normalized shank position relative to the `HAMMER_POSITION_DAMPER` threshold.
2.  **Differentiate:** If Active, calculate the slope ($mm/tick$) over the last 5 samples.
3.  **Smooth:** Pass the slope through a 16-sample SMA to stabilize the value.
4.  **Guard:** If the gate just opened, the `TemporalContinuityGuard` resets buffers to prevent "step-response" artifacts.
5.  **Normalize:** Convert $mm/tick$ to $m/s$ using `TicksToMsNormalizer`.
