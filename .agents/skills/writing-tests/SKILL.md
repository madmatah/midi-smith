---
name: writing-tests
description: Writing unit test files with Catch2 following this project's BDD structure and TDD conventions. Use when adding a new class, writing tests, or asked to test domain logic.
---

# Writing Tests

## Test framework

Tests use **Catch2 v3** (header-only, already available via `FetchContent`).

Common includes:

```cpp
#include <catch2/catch_test_macros.hpp>                          // TEST_CASE, SECTION, REQUIRE
#include <catch2/matchers/catch_matchers_floating_point.hpp>     // WithinAbs, WithinRel
#include <catch2/matchers/catch_matchers_string.hpp>             // ContainsSubstring
```

## File placement

Mirror the source path under `tests/`:

| Source file | Test file |
|---|---|
| `include/domain/path/to/class.hpp` | `tests/domain/path/to/class.test.cpp` |
| `src/dsp/filters/ema_filter.cpp` | `tests/dsp/filters/ema_filter.test.cpp` |
| `src/sensors/sensor_state.hpp` | `tests/sensors/sensor_state.test.cpp` |

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

Two strategies are available. **Use FakeIt by default**; fall back to manual stubs when FakeIt doesn't fit.

### Strategy 1 — FakeIt (default)

Use **FakeIt** to mock `*Requirements` interfaces. It provides concise mock setup, built-in call verification, and avoids writing boilerplate stub classes.

Include and alias:

```cpp
#include <fakeit.hpp>

using fakeit::Mock;
using fakeit::Verify;
using fakeit::When;
```

**Macro workaround** — FakeIt's `Method()` macro conflicts with some toolchains. Always define this alias at the top of the test file:

```cpp
#define fakeit_Method(mock, method) Method(mock, method)
```

Usage pattern:

```cpp
Mock<SomeRequirements> mock;
ClassUnderTest sut(mock.get());

When(fakeit_Method(mock, DoSomething)).Do([](const uint8_t* data, uint8_t len) {
  REQUIRE(len == 3);
  REQUIRE(data[0] == 0x42);
});

sut.Act();

Verify(fakeit_Method(mock, DoSomething)).Once();
```

Common verifications:

```cpp
Verify(fakeit_Method(mock, Method)).Once();          // called exactly once
Verify(fakeit_Method(mock, Method)).Never();          // never called
Verify(fakeit_Method(mock, Method)).Exactly(3);       // called exactly 3 times
When(fakeit_Method(mock, Method)).Return(value);      // stub a return value
When(fakeit_Method(mock, Method)).AlwaysReturn(value); // stub for all calls
```

### Strategy 2 — Manual stubs (when FakeIt doesn't fit)

Fall back to hand-written stubs in these situations:

- **Stateful behavior** — the stub must maintain internal state across calls (e.g. a queue that delivers frames sequentially with an index). A struct is more readable than chaining stateful lambdas.
- **Shared stubs** — the stub is reused across multiple test files. Extract it to a `test_stubs.hpp` with a proper namespace (e.g. `domain::dsp::engine::test`); a struct is more portable than duplicating FakeIt setup.
- **Non-virtual interfaces** — FakeIt operates via vtable manipulation and only works with virtual methods. If the interface uses C++20 concepts or CRTP, use a manual stub.
- **Free functions or static methods** — FakeIt cannot mock these.

Naming conventions:

- `Recording<X>` for stubs that capture calls (e.g. `RecordingKeyActionHandler`).
- `<X>Stub` for stubs that feed canned data (e.g. `FixedQueueStub`).

Rules:

- Place stubs in an anonymous namespace within the test file.
- Stubs implement `*Requirements` interfaces, never wrap concrete classes.

Example:

```cpp
namespace {

struct FixedQueueStub final : public midismith::os::QueueRequirements<Frame> {
  explicit FixedQueueStub(std::vector<Frame> frames) : frames_(std::move(frames)) {}

  bool Send(const Frame&, std::uint32_t) noexcept override { return false; }
  bool SendFromIsr(const Frame&) noexcept override { return false; }

  bool Receive(Frame& item, std::uint32_t) noexcept override {
    if (next_index_ >= frames_.size()) return false;
    item = frames_[next_index_++];
    return true;
  }

 private:
  std::vector<Frame> frames_;
  std::size_t next_index_ = 0;
};

}  // namespace
```

### Decision guide

| Situation | Strategy |
|---|---|
| Mock a `*Requirements` interface | FakeIt |
| Verify call count or arguments | FakeIt |
| Stub a simple return value | FakeIt |
| Stub needs internal state across calls | Manual stub (`*Stub`) |
| Stub is shared across multiple test files | Manual stub (in `test_stubs.hpp`) |
| Interface is non-virtual (concepts, CRTP) | Manual stub |
| Free functions or static methods | Manual stub |
| In doubt | FakeIt |

## TDD rule

Create the test file alongside the implementation, not after.
