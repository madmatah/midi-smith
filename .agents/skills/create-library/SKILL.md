---
name: create-library
description: Creates a new library in libs/ from scratch, with the correct monorepo structure. Use when asked to scaffold a new library package.
---

# Create Library

Use this skill when the task is to create a **new** library from scratch in `libs/`.

## 1. Confirm the target shape

Before creating files, lock these inputs:

1. `library-name-kebab` (folder/include path), e.g. `piano-controller`
2. `library_name_snake` (CMake target + namespace scope), e.g. `piano_controller`
3. Library type:
   - `domain-header-only` — no `.cpp` sources (INTERFACE)
   - `domain-with-src` — has `.cpp` sources (STATIC)
4. Internal dependencies (other `libs/*` targets), if any
5. Test policy:
   - `with-tests` (default for domain libs)
   - `without-tests` (only if explicitly requested)

## 2. Scaffold the directory

### Domain header-only (no tests yet)

```text
libs/<name>/
├── CMakeLists.txt
├── CMakePresets.json
└── include/<name>/
```

### Domain header-only (with tests)

```text
libs/<name>/
├── CMakeLists.txt
├── CMakePresets.json
├── include/<name>/
└── tests/
    ├── CMakeLists.txt
    └── <feature>.test.cpp
```

### Domain with src (always with tests)

```text
libs/<name>/
├── CMakeLists.txt
├── CMakePresets.json
├── include/<name>/
├── src/
│   └── <source>.cpp
└── tests/
    ├── CMakeLists.txt
    └── <feature>.test.cpp
```

## 3. Create `CMakeLists.txt`

`midismith_add_lib` is a macro that handles everything automatically:
- Detects the type: **STATIC** if `SOURCES` is provided, **INTERFACE** otherwise.
- Calls `enable_testing()` when `HOST_TESTS` is ON.
- Calls `add_subdirectory(tests)` if `HOST_TESTS` is ON and `tests/CMakeLists.txt` exists.
- Resolves intra-monorepo dependencies via `DEPENDS` (calls `midismith_require_lib` + `target_link_libraries` automatically).

### Header-only, no dependencies

```cmake
cmake_minimum_required(VERSION 3.22)
project(<library_name_snake> LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME <library_name_snake>)
```

### Header-only, with dependencies

```cmake
cmake_minimum_required(VERSION 3.22)
project(<library_name_snake> LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME <library_name_snake> DEPENDS <dep-name-kebab>)
```

### With sources and dependencies

```cmake
cmake_minimum_required(VERSION 3.22)
project(<library_name_snake> LANGUAGES CXX)

include(${CMAKE_CURRENT_LIST_DIR}/../../cmake/midismith_add_lib.cmake)

midismith_add_lib(NAME <library_name_snake>
    SOURCES
        src/<source>.cpp
    DEPENDS <dep-name-kebab>
)
```

> `DEPENDS` accepts library directory names (kebab-case). Multiple dependencies are space-separated: `DEPENDS midi io config`.

## 4. Create `CMakePresets.json`

All domain libs delegate to the shared preset in `libs/`:

```json
{
  "version": 4,
  "include": ["../CMakePresets.json"]
}
```

> `version: 4` is required for `include` support (CMake ≥ 3.23, project uses 3.28).

## 5. Create public API skeleton

Rules:

1. Public headers live under `include/<library-name-kebab>/...`
2. Public include form: `#include "<library-name-kebab>/..."`
3. Namespace root: `midismith::<library_name_snake>`
4. Interfaces use `*Requirements` naming in `*_requirements.hpp`
5. Code must be self-documenting; comments only for non-obvious rationale

## 6. Create `tests/CMakeLists.txt`

`midismith_add_test` is a macro that handles all test boilerplate automatically:
- Includes Catch2 (and FakeIt if `USE_FAKEIT` is set)
- Creates the `<NAME>_unit_tests` executable
- Links `Catch2::Catch2WithMain`, the lib target, and any extra `LINK_LIBRARIES`
- Sets `UNIT_TESTS=1` compile definition and `cxx_std_20`
- Calls `catch_discover_tests`

### Baseline (Catch2 only)

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME <library_name_snake>
    SOURCES <feature>.test.cpp
)
```

### With FakeIt (when mocking is required)

```cmake
include(${CMAKE_CURRENT_LIST_DIR}/../../../cmake/midismith_add_test.cmake)

midismith_add_test(NAME <library_name_snake>
    SOURCES <feature>.test.cpp
    USE_FAKEIT
)
```

## 7. Optional consumer wiring

Only if explicitly requested by the task:

- **Firmware consumer**: add `midismith_require_lib(<name-kebab>)` in the firmware `CMakeLists.txt`, then link the target.
- **Lib consumer**: add the dependency name to the `DEPENDS` list in the consuming lib's `midismith_add_lib()` call.

If the task is strictly "create from scratch", do not modify firmware or other lib files.

## 8. Validation

Run, in order:

1. **Standalone** (lib in isolation):

```bash
cmake -S libs/<name> --preset Host-Debug
cmake --build libs/<name>/build/Host-Debug
ctest --test-dir libs/<name>/build/Host-Debug --output-on-failure
```

2. **Monorepo host** (no regression):

Use the `running-tests` skill.

3. If consumers were wired, validate impacted firmware presets manually (cross-compilation).

## 9. Done criteria

A new library is complete when:

1. Folder structure matches the chosen type
2. CMake target and namespace use `library_name_snake`
3. Public include path uses `library-name-kebab`
4. `CMakePresets.json` is version 4 with `include: ["../CMakePresets.json"]`
5. Tests pass in both standalone and monorepo builds
