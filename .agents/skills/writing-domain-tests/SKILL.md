---
name: writing-domain-tests
description: Creates Catch2 unit test files for domain layer classes following this project's BDD structure and TDD conventions. Use when adding a new class to `domain/`, writing tests for a domain component, or asked to test domain logic.
---

# Writing Domain Tests

## File placement

Mirror the domain source path under `tests/domain/`:

| Domain file | Test file |
|---|---|
| `domain/src/dsp/filters/ema_filter.cpp` | `tests/domain/dsp/filters/ema_filter.test.cpp` |
| `domain/src/sensors/sensor_state.hpp` | `tests/domain/sensors/sensor_state.test.cpp` |

## File structure

```cpp
#if defined(UNIT_TESTS)

#include "domain/path/to/class.hpp"

#include <catch2/catch_test_macros.hpp>
// Add matchers as needed, e.g.:
// #include <catch2/matchers/catch_matchers_floating_point.hpp>

namespace {
// Stubs and mocks here (if needed)
}  // namespace

TEST_CASE("The ClassName class") {
  SECTION("The MethodName() method") {
    SECTION("When <context>") {
      SECTION("Should <expected outcome>") {
        // Arrange
        // Act
        // Assert
      }
    }
  }
}

#endif
```

## BDD hierarchy (4 levels, mandatory)

| Level | Pattern | Example |
|---|---|---|
| `TEST_CASE` | `"The <Name> class"` or `"The <Name> struct"` | `"The EmaFilterRatio class"` |
| `SECTION` L1 | `"The <Method>() method"` | `"The Transform() method"` |
| `SECTION` L2 | `"When <context>"` | `"When alpha is 0"` |
| `SECTION` L3 | `"Should <outcome>"` | `"Should keep the first value"` |

Inside the leaf `SECTION`: Arrange, then Act, then Assert. No comments on these phases.

## Stubs and mocks

- Place stubs in an anonymous namespace within the test file.
- Use `Recording<X>` naming for stubs that capture calls (e.g. `RecordingKeyActionHandler`).
- If stubs are shared across multiple test files in the same directory, extract them to a `test_stubs.hpp` with a proper namespace (e.g. `domain::dsp::engine::test`).
- Stubs implement `*Requirements` interfaces, never wrap concrete classes.

## TDD rule

Create the test file alongside the implementation, not after.
