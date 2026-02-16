## Theory of Operation: Physical Position Stage

**Responsibility:** Map the normalized shank position $[0.0, 1.0]$ to a physical distance in millimeters ($mm$).

### 1. The Scaling Principle

The `AnalogSensorLinearizer` stage provides a **Normalized Position** where the values are mapped as follows:
* **1.0**: Hammer at **Rest** (maximum distance from sensor).
* **0.0**: Hammer at **Strike** (minimum distance from sensor, hitting the string).

The `PhysicalPositionStage` scales the normalized position to a physical coordinate in $mm$, representing the **shank's distance relative to the strike point**.

### 2. Physical Calibration ($\Delta d$)

We define $\Delta d$ as the total physical travel of the shank at the specific point where the sensor is positioned.

* $D_{rest}$: Physical distance between sensor and shank at rest.
* $D_{strike}$: Physical distance between sensor and shank at strike point.

The total travel is:
$$\Delta d = D_{rest} - D_{strike}$$

**Example Calibration:**
* If $D_{rest} = 8.0\text{ mm}$ and $D_{strike} = 1.9\text{ mm}$
* Then $\Delta d = 6.1\text{ mm}$

### 3. Implementation

The `PhysicalPositionStage` uses a `LinearScaler<Delta_d_mm>`. Since the input is normalized ($1.0$ to $0.0$), the output becomes a physical "remaining distance" in $mm$.

* **Input:** Normalized Position (1.0 = Rest, 0.0 = Strike).
* **Operator:** Multiplication by $\Delta d$.
* **Output:** Physical Position in $mm$ ($6.1\text{ mm}$ = Rest, $0.0\text{ mm}$ = Strike).

### 4. Impact on subsequent calculations

By setting the strike point as the **zero reference**, all subsequent physical calculations become intuitive. A position of $2.0\text{ mm}$ means the hammer is exactly $2\text{ mm}$ away from the string, regardless of how high or low the sensor itself is mounted.

Because the strike point is defined as **0.0**, moving toward the string means the position value is decreasing. Consequently:
* **Downward motion** (toward strike) results in a **negative** velocity.
* **Upward motion** (return/rebound) results in a **positive** velocity.