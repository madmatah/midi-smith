## Theory of Operation: Hammer Speed Differentiator

**Responsibility:** Compute real-time physical **speed** from linearized position.

### Overview

The `SlidingLinearRegression` (aka `HammerSpeedEstimator`) is a DSP stage responsible for calculating the instantaneous speed of the hammer (). This is a continuous stream of data used for state tracking, direction sensing, and as the primary input for the MIDI engine.

### Algorithm: Sliding Window Linear Regression

To mitigate sensor noise and quantization jitter from the ADC at 3500 Hz, we avoid simple finite differences. Instead, we use an **Ordinary Least Squares (OLS) Linear Regression** on a fixed window of  samples.

* **Window Size ():** Recommended  to . At 3500 Hz, 5 samples represent .
* **Computation:** At each sample, the stage fits a line over the last  points. The slope represents the **instantaneous speed**.

### Operating Principles

1. **Continuous Calculation:** The differentiator runs regardless of the hammer position to provide a constant "sense of motion."
2. **Sign Convention:** * : Hammer moving toward the string (descending in normalized coordinates ).
* : Hammer returning toward the rest position (ascending ).


3. **Output:** A signed floating-point value representing the rate of displacement.
