## Theory of Operation: Hammer Kinematics Stage

**Responsibility:** Translate the physical velocity of the shank at the sensor point into the actual velocity of the hammer head ($m/s$).

### 1. The Leverage Principle ($L_{ratio}$)

The optical sensor is positioned close to the hammer's pivot to capture the full range of motion. However, the hammer head (which strikes the string) is located much further from the pivot. Due to this angular geometry, the hammer head travels a much larger arc than the shank at the sensor's location.

We define the **Leverage Ratio** as:
$$L_{ratio} = \frac{R_{hammer}}{R_{sensor}}$$

* $R_{hammer}$: Distance from the pivot to the center of the hammer head (typically $\approx 120\text{ mm}$ to $130\text{ mm}$).
* $R_{sensor}$: Distance from the pivot to the sensor's optical axis (typically $\approx 15\text{ mm}$ to $20\text{ mm}$).

The hammer head moves exactly $L_{ratio}$ times faster than the point monitored by the sensor.



### 2. Physical Velocity Mapping

At this stage in the workflow, the `ShankSpeedEstimator` has already produced a velocity in meters per second ($m/s$), but this value represents the speed **at the sensor point**.

To obtain the final hammer head speed ($V_{hammer}$):
$$V_{hammer} = V_{shank} \times L_{ratio}$$

**Example Calibration:**
* $R_{hammer} = 126.0\text{ mm}$
* $R_{sensor} = 17.5\text{ mm}$
* $L_{ratio} = 7.2$

If the shank speed is measured at **$0.5\text{ m/s}$**, the actual hammer head speed striking the string will be **$3.6\text{ m/s}$**.

### 3. Implementation

The workflow utilizes the `ShankToHammerKinematicsStage` (a `LinearScaler<L_ratio>`).

* **Input:** Physical Shank Velocity ($m/s$ at sensor point).
* **Operation:** Multiply by $L_{ratio}$.
* **Output:** Final Hammer Velocity ($m/s$ at hammer head).

### 4. Why this matters

Using a physical kinematic model instead of arbitrary units allows the system to match academic models and empirical measurements of piano acoustics. Most research regarding piano touch and velocity (e.g., Goebl et al.) defines "strike velocity" as the speed of the hammer head in $m/s$. Having this real-world value is essential for accurate MIDI velocity mapping and consistent tone reproduction.
