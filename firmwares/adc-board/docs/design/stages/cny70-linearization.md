# CNY70 linearization design

## 1. Foundations: Calibration Settings and Sensor Signature
The system relies on the correlation of two data sources prior to any processing:

* **Sensor Response Curve (Physics)**: A reference curve I_rel = f(Dist) derived from the datasheet, defined over the measured distance range of the response curve (from 0.0 mm to 10.0 mm).
* **Mechanical Parameters (The Piano)**: Targeted distances defined by the physical assembly:
    * **D_rest**: The distance between the sensor (fixed) and the hammer shank when the hammer is in its resting position (typical value: 10.0 mm).
    * **D_strike**: The distance between the sensor and the hammer shank at the moment of impact with the string (typical value: 2.0 mm).
    * *Constraints*: D_strike >= min distance of the response curve and D_rest > D_strike.

## 2. Individual Calibration (Amplitude Normalization)
For each sensing unit, the system records actual current values in mA at the parameterized distances. This step compensates for the gain dispersion between components:
* **I_rest**: Current measured at D_rest (minimum threshold).
* **I_strike**: Current measured at D_strike (maximum threshold).

## 3. Boot-time Generation (Current <-> Position Mapping)
During initialization, the firmware generates a dedicated Look-Up Table (LUT) for each of the 22 sensors on the board. This process is based on a **projection**:
* The system extracts the relevant "working window" from the Master Signature between the D_rest and D_strike boundaries.
* It maps the actual current values (I_rest ... I_strike) to a linear **Normalized Position**, defined within the interval [0.0, 1.0] (where 0.0 = strike and 1.0 = rest).

## 4. Runtime Execution (Performance)
Real-time processing is optimized using the pre-calculated LUTs:
* The Processor receives the instantaneous current in mA.
* It performs a **Piecewise Linear Interpolation** on the sensor-specific LUT.
* **Result**: A linear position [0.0, 1.0] (0=strike, 1=rest), mechanically normalized and optically corrected, transmitted to the velocity engine.
