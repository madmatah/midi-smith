*Rules applying to packages in `libs/`. See also @AGENTS.md for monorepo-wide rules.*

---

## L.0 Package Structure

**Domain lib** (standard case — `dsp`, `midi`, `piano`…):

```
<lib-name>/
│
├── include/<lib-name>/           # Public headers — namespace midismith::<lib-name>::
│   └── <sub-domain>/             # Optional sub-domain grouping (e.g. filters/, math/)
├── src/                          # Implementation files (.cpp)
├── tests/                        # Host-only unit tests
└── CMakeLists.txt                # Standalone target with project() and Host-Debug preset
```

**Infrastructure lib** (`os`, `bsp`):

```
<lib-name>/
│
├── include/<lib-name>/           # Public headers — namespace midismith::<lib-name>::
├── src/                          # Implementation files (.cpp)
└── CMakeLists.txt                # No project(), no standalone preset.
                                  # Consumed via add_subdirectory() from a firmware.
                                  # No tests/ — FreeRTOS/HAL not available outside firmware context.
```

### Namespace

**Domain libs** (standard case — `dsp`, `midi`, `piano`…): `midismith::<scope>::<sub-domain>` — no layer level (the entire lib is the domain).

| Level | Value | Purpose | Example |
|-------|-------|---------|---------|
| **1 — Root** | `midismith` | Product root, always present | `midismith::` |
| **2 — Scope** | lib name | Identifies the library | `midismith::dsp::` |
| **3 — Sub-domain** | functional area (optional) | `filters`, `math`, `engine`… | `midismith::dsp::filters::` |

**Infrastructure libs** (`os`, `bsp`) : `midismith::<scope>::` — scope = lib name, no additional sub-domain expected.

Namespaces mirror the directory structure.

| Directory | Namespace |
|-----------|-----------|
| `libs/dsp/include/dsp/filters/` | `midismith::dsp::filters` |
| `libs/midi/include/midi/` | `midismith::midi` |
| `libs/os/include/os/` | `midismith::os` |

---

## L.1 Package Contract

**Domain libs** :
- **`include/<lib-name>/` root**: all public headers under `include/<lib-name>/`. Consumed via `#include "<lib-name>/..."`.
- **No HAL/FreeRTOS/BSP dependency**: no headers from `Core/`, `Drivers/`, `Middlewares/`, or any infra lib implementation. May use `*Requirements` interfaces.
- **Standalone buildable**: compiles and runs tests with a host C++ compiler; no cross-compilation required.
- **Single domain concern**: one clearly scoped responsibility per lib. No mixing of unrelated domains in the same package.
- **CMake**: standalone target with `project()` and a `Host-Debug` preset.

**Infrastructure libs** (`libs/os`, `libs/bsp`) :
- Same `include/<lib-name>/` root convention.
- **Depend on CubeMX-generated headers**: FreeRTOS or STM32 HAL headers must be provided by the consuming firmware's include paths.
- **Not standalone**: cannot be configured or built independently; must be consumed by a firmware `CMakeLists.txt`.
- **CMake**: no `project()`, no standalone preset — consumed via `add_subdirectory()` from a firmware.

---

## L.2 Code Rules

All code follows the domain rules defined in §2.3 of @AGENTS.md. Unlike firmware packages where these rules apply only to `domain/`, they apply here to the entire package.
