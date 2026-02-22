# C++ style rules (project-owned code)

## Reference

This project follows the Google C++ Style Guide for project-owned code.

## Scope

Applies to:
- `App/`, `BSP/`, `OS/`, `Domain/`, `tests/`

Does not apply to:
- `Core/`, `Drivers/`, `Middlewares/` (STM32CubeMX generated)
- `Third_Party/` (vendored third-party code)

## Naming and layout

For project-owned code:
- Directories use `lowercase_with_underscores/`
- Files use `lowercase_with_underscores.hpp` and `lowercase_with_underscores.cpp`
 - C API boundaries may use `.h` (e.g. headers included by STM32CubeMX-generated `.c` files)

Symbol naming follows the Google C++ Style Guide:
- Types (classes/structs/enums): `UpperCamelCase` (PascalCase)
- Functions (including class methods): `PascalCase()`
- Accessors and mutators: may be `snake_case()`
- Variables: `snake_case`
- Class data members: `snake_case_`
- Constants: `kUpperCamelCase`
- Namespaces: `lowercase`

Interface naming avoids Hungarian notation:
- Interfaces (pure abstract types) use the `*Requirements` suffix, not an `I*` prefix
- Interface headers use `lowercase_with_underscores_requirements.hpp`

Example:
- `LoggerRequirements` in `logger_requirements.hpp`

## Formatting

Formatting is enforced with clang-format using [`.clang-format`](../.clang-format) (Google-based, `ColumnLimit: 100`).

Generated code and vendored code must never be reformatted. Exclusions are defined in [`.clang-format-ignore`](../.clang-format-ignore).

### Run formatting

If you build with CMake in the repository, use the `format` target:

```bash
cmake --build build/current --target format
```

## Lint (cpplint)

This project uses `cpplint` to enforce Google-style checks on project-owned code only.

Install (if needed):

```bash
python3 -m pip install --user cpplint
```

Run lint:

```bash
cmake --build build/current --target lint
```
