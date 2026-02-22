
## Coding Style

- **Reference**: Google C++ Style Guide, with project-level constraints documented below.
- **Scope**: All `firmwares/` and `libs/` code. Excludes `{Core,Drivers,Middlewares,USB_DEVICE}/` and `third_party/`.
- **Language standard**: C++20.
- **Formatting**: clang-format, Google-based, `ColumnLimit: 100`. Excludes generated/vendor code via `.clang-format-ignore`.

**— File and directory naming**
- Directories: `lowercase_with_underscores/`
- Files: `lowercase_with_underscores.hpp` and `lowercase_with_underscores.cpp`
- C API boundaries may use `.h` (e.g. headers included by STM32CubeMX-generated `.c` files)

**— C++ symbol naming**
- Types (classes/structs/enums): `UpperCamelCase`
- Functions (including class methods): `PascalCase()`
- Accessors/mutators: `snake_case()` (e.g. `count()`, `set_count(...)`)
- Variables: `snake_case`
- Class data members: `snake_case_` (trailing underscore)
- Constants: `kUpperCamelCase`
- Namespaces: `lowercase`
- Interfaces: `*Requirements` (e.g. `LoggerRequirements`) in `*_requirements.hpp`

### Namespace Hierarchy

Two patterns exist depending on the package type — see @firmwares/AGENTS.md for firmware packages and @libs/AGENTS.md for library packages.

Universal rules (apply to all packages):
- `midismith::` is never omitted, even in internal files.
- `using namespace midismith;` in header files is forbidden.

---

# Naming & Self-Documentation

**— Intention-Oriented Naming**: names based on intent, not implementation.
- Good: `BeginTransaction()`, `EnableDirectAccess()`, `WaitUntilReady()`
- Less good: `Select()`, `SetMemoryMappedMode()`, `CheckStatus()`

**— Explicit over Concise**: long, precise names are preferred over short, ambiguous ones.
- Good: `remaining_bytes_to_write`, `current_buffer_index`, `sector_start_address`
- Less good: `rem`, `idx`, `addr`

**— Zero-Comment Policy**: code must be self-documenting; comments explain "why", not "what".
- If logic seems complex, extract it into a well-named function or rename the variables.
- Function names must use action verbs (e.g., `Start`, `Stop`, `Configure`, `Validate`).
- Exceptions: hardware constraint justification (e.g., datasheet delays) and config file value documentation.

**— Domain Vocabulary**: use precise domain terms (`page_size`, `sector_erase`, `quad_mode`).

**— Avoid Abbreviations**: unless a universal industry standard (e.g., `SPI`, `DMA`, `ISR`).
- Good: `configuration`, `transmit`, `receive`, `callback` · Less good: `cfg`, `tx`, `rx`, `cb`

**— Unit Suffixes**: always append units to time/frequency/size variables if not implicit.
- Good: `timeout_ms`, `frequency_hz`, `buffer_size_bytes` · Less good: `timeout`, `freq`, `size`

---

## 1. Project Organization

### 1.1 Target Directory Structure

```
monorepo-root/
│
├── firmwares/                    # Firmware packages (STM32, cross-compiled, CubeMX-managed)
│   ├── adc-board/                # Firmware — STM32H743
│   └── main-board/               # Firmware — STM32H7B0
│
├── libs/                         # Shared library packages (host-portable, domain code)
│   ├── common/                   # Transitional catch-all — will be decomposed into focused libs
│   ├── os/                       # (future) FreeRTOS abstraction — infra, not standalone
│   ├── bsp/                      # (future) Hardware abstraction — infra, not standalone
│   └── <domain-lib>/             # (future) e.g. dsp, midi, piano — pure domain, standalone
│
├── tools/                        # Debug and development host tools
├── third_party/                  # Vendored external code (unmodified)
│
├── CMakeLists.txt                # Monorepo orchestrator (routing + tooling targets)
├── CMakePresets.json             # Build presets for all packages
├── CPPLINT.cfg                   # cpplint config shared across all packages
├── .clang-format
├── .clang-format-ignore
└── .clangd
```

### 1.2 Organization Rules

**— Firmware packages live in `firmwares/`**: cross-compiled for STM32 targets; managed by CubeMX; depend on HAL and FreeRTOS. See @firmwares/AGENTS.md §F.0 for structure.

**— Domain library packages live in `libs/`**: pure C++, no CubeMX dependency, buildable and testable standalone on a host machine. Each lib has a single well-defined domain concern. See @libs/AGENTS.md §L.0 for structure.

**— Infrastructure library packages** (`libs/os`, `libs/bsp`): depend on CubeMX-generated headers (FreeRTOS, HAL); cannot be built standalone; always consumed by a firmware package that provides the generated headers.

**— No first-party source at the repository root**: the root contains only orchestration (`CMakeLists.txt`), shared tooling, and `third_party/`.

**— Vendored external code lives in `third_party/`**: never modified, not subject to project style rules. Updates by replacing the vendored subtree.

**— Root `CMakeLists.txt` is an orchestrator and tooling host**: no application logic. Routes firmware builds; defines monorepo-wide tooling targets (`lint`, `format`, `format_check`). See §4.1 for details.

**— Shared tooling at the repository root**: `.clang-format`, `.clang-format-ignore`, `.clangd`, `CPPLINT.cfg` are shared across all packages. A package may override locally.

**— Any deviation from this structure must be justified.**

---

## 2. Software Architecture

### 2.1 Clean Code Principles

**— Follow SOLID principles**: Single Responsibility, Open/Closed, Liskov Substitution, Interface Segregation, Dependency Inversion.
**— Each class has a single responsibility**: one class = one clearly defined role.
**— Prefer composition over inheritance**: build behavior through object composition, not deep hierarchies.
**— Dependencies flow inward**: high-level modules never depend on low-level modules; both depend on abstractions.
**— ISP**: depend on the smallest possible interface; never inject a "Global Board" when only one peripheral is needed.
**— LSP**: derived classes must respect the interface contract (e.g., a non-blocking interface must never have a blocking implementation).

### 2.2 Dependency Inversion Principle (DIP)

**— Depend on abstractions, not concrete implementations**: application code depends on interfaces (abstract classes).
**— Interfaces are owned by the client**: define them in the module that uses them, not in the implementation.
**— Constructor injection**: pass dependencies as `&` references — guarantees non-null and makes dependencies explicit.

#### Example: Interface Definition

```cpp
// bsp/include/bsp/gpio_requirements.hpp
class GpioRequirements {
public:
    virtual ~GpioRequirements() = default;
    virtual void set() = 0;
    virtual void reset() = 0;
    virtual void toggle() = 0;
    virtual bool read() const = 0;
};
```

#### Example: Component Depending on Interface

```cpp
// app/include/app/tasks/led_task.hpp
#include "bsp/gpio_requirements.hpp" // Depends on interface, not implementation

class LedTask : public Task {
public:
    explicit LedTask(GpioRequirements& led_hardware) : led_(led_hardware) {}

    void run() override {
        led_.toggle();
    }

private:
    GpioRequirements& led_;
};
```

### 2.3 Domain Code Rules

*Applies to `domain/` within firmware packages and to library packages as a whole.*

- No FreeRTOS, HAL, or BSP dependencies (`#include` of generated files is forbidden).
- Must be compilable and testable on a host machine.
- Depends only on the C++ standard library or explicitly approved portable dependencies.
- No hardware types (`uint32_t*` to registers) or OS primitives.
- Prefer deterministic, side-effect-free functions.

---

## 3. Embedded C++ Constraints

All packages comply with these constraints, since all code is ultimately compiled for or alongside embedded targets.

- **No exceptions**: do not depend on exception handling.
- **No RTTI**: no `dynamic_cast` or `typeid`.
- **Controlled allocation**: `new`/`delete` avoided or strictly framed; allocation at initialization only.
- **No complex static initializations**: no dependency on global or static initialization order.

---

## 4. Build & Tools

### 4.1 CMake

CMake is the source of truth for the build; no critical setting depends on the IDE.

- Root `CMakeLists.txt`: routes via `MIDISMITH_ACTIVE_FIRMWARE` (`adc`, `master`) or `HOST_TESTS=ON`. Defines monorepo-wide tooling targets (`lint`, `format`, `format_check`) covering all project-owned code. No application logic.
- Package-level CMake rules: see @firmwares/AGENTS.md §F.7 and @libs/AGENTS.md §L.1.

**Presets** (`CMakePresets.json` at root):
- `adc-Debug`, `adc-Release` — cross-compile adc-board
- `main-Debug`, `main-Release` — cross-compile main-board
- `Host-Debug` — host compiler, `HOST_TESTS=ON`

---

## 5. Quality & Maintainability

- Any rule violation must be explicitly documented; no implicit exceptions.

---

## 6. Error Handling

- Error propagation strategy must be defined and consistent (return codes, callbacks, or assertions).
- Assertions help catch violations early in development builds.

---

## 7. Testing & Validation

- Business logic must be testable without hardware.
- CI builds both firmware targets and host tests.

**— TDD-Centric**: any new class or logic (except in `bsp/` or `os/`) MUST have a corresponding unit test. The agent must not consider a task "Done" if the test file is missing. Tests must be written before or during implementation.

**— Logic-only tests**: must never depend on HAL, FreeRTOS, or BSP. Use mocks (inheritance or minimal stubs) for any `Requirements` interfaces.

### 7.1 Test Organization

Each package owns its tests at `firmwares/<name>/tests/` or `libs/<name>/tests/`. Tests are built as host executables and never linked into firmware.

---

*Package-specific rules: Firmware → @firmwares/AGENTS.md · Libraries → @libs/AGENTS.md*
