
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

Two patterns exist depending on the package type:

**Firmware packages** (`firmwares/`): `midismith::<scope>::<layer>::<sub-domain>` (sub-domain may be omitted).

| Level | Value | Purpose | Example |
|-------|-------|---------|---------|
| **1 — Root** | `midismith` | Product root, always present | `midismith::` |
| **2 — Scope** | `adc_board` / `main_board` | Code specific to one firmware | `midismith::adc_board::` |
| **3 — Layer** | `app`, `domain`, `bsp`, `os` | Architectural role of the code | `midismith::adc_board::domain::` |
| **4 — Sub-domain** | `shell`, `config`, `storage`, … | Functional domain | `midismith::adc_board::app::shell::` |

**Library packages** (`libs/`): `midismith::<scope>::<layer>::<sub-domain>`.

Multi-layer libraries (e.g. `common`) retain the layer level. Future single-domain libraries (e.g. `dsp`, `midi`) omit it: `midismith::<scope>::<sub-domain>`.

| Level | Value | Purpose | Example |
|-------|-------|---------|---------|
| **1 — Root** | `midismith` | Product root, always present | `midismith::` |
| **2 — Scope** | `common` (+ future libs) | Shared library | `midismith::common::` |
| **3 — Layer** | `app`, `domain`, `bsp`, `os` | Present only when the library has multiple layers | `midismith::common::os::` |
| **4 — Sub-domain** | `shell`, `config`, `storage`, … | Functional domain | `midismith::common::domain::storage::` |

Namespaces mirror the directory structure. Scope names use underscores (`adc-board` → `adc_board`).

| Directory | Namespace |
|-----------|-----------|
| `libs/common/include/domain/storage/` | `midismith::common::domain::storage` |
| `libs/common/include/os/` | `midismith::common::os` |
| `firmwares/adc-board/app/include/app/shell/` | `midismith::adc_board::app::shell` |
| `firmwares/adc-board/domain/include/domain/` | `midismith::adc_board::domain` |
| `firmwares/main-board/bsp/include/bsp/can/` | `midismith::main_board::bsp::can` |

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
│   └── common/                   # Shared library (OS abstractions, BSP interfaces, domain types)
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

**— Firmware packages live in `firmwares/`**: cross-compiled for STM32 targets; managed by CubeMX; depend on HAL and FreeRTOS. See §F.0 for structure.

**— Shared library packages live in `libs/`**: host-portable; no CubeMX dependency; cannot be built standalone (they are consumed by firmware packages). See §L.0 for structure.

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

**Two-level structure**:
- Root `CMakeLists.txt`: routes via `MIDISMITH_ACTIVE_FIRMWARE` (`adc`, `master`) or `HOST_TESTS=ON`. Defines monorepo-wide tooling targets (`lint`, `format`, `format_check`) covering all project-owned code. No application logic.
- `firmwares/<firmware>/CMakeLists.txt`: self-contained, knows nothing about the monorepo root.
- `libs/common/CMakeLists.txt`: defines a linkable CMake library target.

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

# Firmware Packages

*Rules applying exclusively to `firmwares/adc-board` and `firmwares/main-board`.*

---

## F.0 Package Structure

```
<firmware-package>/
│
├── app/                          # Application logic (Tasks, Orchestration)
│   ├── include/app/
│   └── src/
├── bsp/                          # Hardware abstraction (C++)
│   ├── include/bsp/
│   └── src/
├── domain/                       # Firmware-specific business logic
│   ├── include/domain/
│   └── src/
├── os/                           # FreeRTOS abstraction (C++)
│   ├── include/os/
│   └── src/
├── tests/                        # Host-only unit tests
│   └── domain/
├── Core/                         # Generated by CubeMX (HAL, startup)
├── Drivers/                      # Generated (HAL, CMSIS) — read-only
├── Middlewares/                  # Generated (FreeRTOS)
├── hardware/svd/                 # SVD file for debugging (non-build resource)
├── CMakeLists.txt
├── CMakePresets.json
└── <package-name>.ioc
```

---

## F.1 CubeMX & Regenerability

- CubeMX generates only hardware infrastructure (HAL, CMSIS, startup, FreeRTOS config).
- Regeneration is always safe: it must never delete or invalidate application code.
- Generated code implements no business logic; all application logic lives outside generated files.
- User code lives in directories unknown to CubeMX (`app/`, `bsp/`, `domain/`, `os/`).
- No CubeMX-generated file is modified outside `USER CODE` zones.

---

## F.2 Language & Interoperability

- HAL and CMSIS remain in C; never rewritten in C++.
- Any C function called from C++ is protected by `extern "C"`.

---

## F.3 Layered Architecture

### F.3.1 Application Layer (App/)
- Never depends on the HAL.
- Contains FreeRTOS tasks and synchronization logic.
- Depends only on BSP/OS interfaces; never includes concrete BSP (except in `Application.cpp`).

### F.3.2 BSP Layer (Board Support Package)
- Only layer allowed to depend on the HAL.
- Thread-safe; concurrent accesses explicitly protected.
- No business logic; no blocking FreeRTOS calls.
- Each peripheral encapsulated in a dedicated C++ class.

### F.3.3 OS Layer (FreeRTOS Abstraction)
- FreeRTOS is never used directly in the application.
- APIs encapsulated in C++ wrappers (tasks, queues, mutexes, timers).

---

## F.4 Composition Root & Subsystems

All object instantiation and wiring happens in the Composition Root, scoped to:
- `firmwares/<firmware>/app/src/application.cpp`
- `firmwares/<firmware>/app/src/composition/*.cpp`
- `firmwares/<firmware>/app/include/app/composition/*.hpp`

Only Composition Root files may include concrete BSP classes.

```cpp
// app/src/application.cpp
#include "bsp/stm32_gpio.hpp" // Concrete class known only here

void Application::boot() {
    static Stm32Gpio status_led(GPIOA, GPIO_PIN_5);
    static LedTask led_task(status_led);
    led_task.start();
}
```

**— Subsystem composition**: each subsystem gets `app/src/composition/<name>_subsystem.cpp`; entry points in `app/include/app/composition/subsystems.hpp`.

**— Anti-pass-through rule**: no "mega context" (`AppContext` with dozens of fields). Prefer minimal capability contexts (`LoggingContext { LoggerRequirements& }`). Subsystems accept only the contexts they need. No service locators.

**— Inside composers**: concrete BSP headers and static allocation are allowed. All other code uses `*Requirements` interfaces.

**— Cross-subsystem**: shared dependencies are created once in the Composition Root and passed via a minimal context.

**— Configuration-driven wiring**: config headers under `app/include/app/config/` stay data-only. Compile-time validation in `app/include/app/config/<name>_validation.hpp`, included by the relevant composer.

---

## F.5 Configuration Management

All paths relative to `firmwares/<firmware>/`:
- `app/include/app/config/config.hpp`: global config (defaults, task stack/priorities, system-wide constants).
- `app/include/app/config/*.hpp`: subsystem-specific config (data-only, human-edited).
- `app/include/app/config/*_validation.hpp`: compile-time validation companion headers.
- Use `constexpr` constants; avoid scattered `#define`.
- All numeric values must be named constants — no magic numbers.
- Board variants handled through build-time configuration.

---

## F.6 FreeRTOS

### F.6.1 Entry Point & Scheduler
- `main` is an initialization point only (hardware init → OS → scheduler). No application loop.
- The scheduler is launched only once; no code after `vTaskStartScheduler()`.

### F.6.2 Task Organization
- Each functional domain is isolated in a task (communication, I/O, display, logic).
- Tasks communicate via queues, notifications, or event groups — never global variables.
- Non-default priorities must be justified and documented.
- Constructors do not create tasks; task creation is centralized and explicit.

### F.6.3 Task Structure
- Each task has its own header + source files and lives in `app/`.
- Task code is never placed in `main`, `Core/`, or `os/`.
- One task = one clearly identified file pair; names must be explicit and stable.

### F.6.4 Interrupts
- ISRs are minimal: notify or post messages only.

### F.6.5 Synchronization & Critical Sections
- Every shared resource has a defined protection mechanism (mutex or critical section).
- Critical sections are kept as short as possible.
- Lock ordering is documented to prevent deadlocks and priority inversion.

### F.6.6 CubeMX Configuration
- FreeRTOS configuration done only via CubeMX; no manual modification of generated files.
- Extensions go through `USER CODE` hooks only.

---

## F.7 Organization Rules

- `Drivers/` is strictly read-only.
- `USB_DEVICE/` is CubeMX-generated; modifications only within `USER CODE` zones.
- `hardware/` contains non-build resources (SVD, datasheets, schematics); never included in the build.
- C, C++, and ASM languages must all be declared in the firmware `CMakeLists.txt`.

---

## F.8 Quality & Maintainability

- BSP is the only layer dependent on the MCU; hardware coupling is minimal and localized.
- The application core is independent of CubeMX.
- C++ abstractions are mockable; the HAL is never mocked directly.

---

## F.9 Error Handling & Diagnostics

- Critical errors are logged before system halt (UART, SWO, or LED codes).
- HardFault, MemManage, BusFault handlers are implemented and provide diagnostics.

---

## F.10 Testing

- Integration tests validate BSP behavior with real or simulated peripherals.
- BSP interfaces can be mocked for off-target testing.

---

# Library Packages

*Rules applying to `libs/common` and future library packages in `libs/`.*

---

## L.0 Package Structure

```
<library-package>/
│
├── include/                      # Single public include root (consumable by other packages)
│   ├── domain/                   # Shared domain types and logic
│   ├── bsp/                      # Shared BSP interfaces (*Requirements)
│   └── os/                       # Shared OS abstractions
├── src/                          # Implementation files (.cpp)
└── tests/                        # Host-only unit tests
```

---

## L.1 Package Contract

- **Single `include/` root**: all headers consumable by other packages live under `include/`. No per-layer nesting.
- **No HAL/FreeRTOS/BSP dependency**: no headers from `Core/`, `Drivers/`, `Middlewares/`, or firmware-specific BSP. May use `*Requirements` interfaces from `libs/common/`.
- **Host-portable**: must compile with the system C++ compiler; no embedded toolchain or architecture assumptions.

---

## L.2 Code Rules

All code follows the domain rules defined in §2.3. Unlike firmware packages where these rules apply only to `domain/`, they apply here to the entire package.
