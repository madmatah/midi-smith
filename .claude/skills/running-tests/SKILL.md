---
name: running-tests
description: Runs the unit test suite for this embedded C++ project on the host machine using CMake and ctest. Use when asked to run tests, execute tests, build tests, or verify that tests pass.
---

# Running Tests

## 1. Save the active firmware preset

Read `build/current` before switching to `Host-Debug`:

```bash
PREVIOUS_PRESET=$(basename "$(readlink build/current 2>/dev/null)")
```

## 2. Configure, build, and run tests

```bash
cmake --preset Host-Debug
cmake --build --preset Host-Debug --target unit_tests
ctest --test-dir build/Host-Debug --output-on-failure
```

## 3. Restore the firmware preset

Run unconditionally, even if tests failed:

```bash
[ -n "$PREVIOUS_PRESET" ] && [ "$PREVIOUS_PRESET" != "Host-Debug" ] && cmake --preset "$PREVIOUS_PRESET"
```

## Run a specific test

Use `-R` with the `TEST_CASE` name to filter, then apply the same save/restore flow:

```bash
ctest --test-dir build/Host-Debug -R "The MidiPiano class" --output-on-failure
```
