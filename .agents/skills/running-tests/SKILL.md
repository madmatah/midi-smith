---
name: running-tests
description: Runs the unit test suite for this embedded C++ project on the host machine using CMake and ctest. Use when asked to run tests, execute tests, build tests, or verify that tests pass.
---

# Running Tests

Use the scripts in this skill. They configure/build with `Host-Debug`, run tests, and restore the previous preset automatically.
Paths below are relative to the skill directory.

## 1. Run all host tests

```bash
bash scripts/run_all_host_tests.sh
```

Optional: pass extra arguments to `ctest`:

```bash
bash scripts/run_all_host_tests.sh --verbose
```

## 2. Run a specific test by description/regex

```bash
bash scripts/run_host_test_by_name.sh "The MidiPiano class"
```

Optional: pass extra arguments to `ctest`:

```bash
bash scripts/run_host_test_by_name.sh "MidiPiano|AsyncTaskMidiController" --verbose
```
