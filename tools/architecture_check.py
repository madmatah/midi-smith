#!/usr/bin/env python3
"""Monorepo architecture guard.

Enforces the mechanically checkable rules of AGENTS.md, firmwares/AGENTS.md and libs/AGENTS.md:
layer purity, composition-root confinement, domain purity, namespace-to-directory mirroring and
the per-package test contract. Rules that need judgement (naming intent, DIP quality, buffer
sizing, test decisiveness) are out of scope here and belong to the /qa reviewers.

Usage:
    python3 tools/architecture_check.py [--repo-root PATH]
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass
from pathlib import Path

SOURCE_SUFFIXES = (".cpp", ".hpp", ".h")

SCANNED_GLOBS = (
    "firmwares/*/app/**/*",
    "firmwares/*/bsp/**/*",
    "firmwares/*/os/**/*",
    "firmwares/*/domain/**/*",
    "firmwares/*/tests/**/*",
    "libs/*/include/**/*",
    "libs/*/src/**/*",
    "libs/*/tests/**/*",
)

INCLUDE_PATTERN = re.compile(r'^\s*#\s*include\s*[<"]([^>"]+)[>"]', re.MULTILINE)
NAMESPACE_PATTERN = re.compile(r"^\s*namespace\s+([A-Za-z_][A-Za-z0-9_:]*)\s*\{", re.MULTILINE)

HAL_INCLUDE_PATTERN = re.compile(r"^(stm32|cmsis|core_cm|system_stm32|main\.h$)", re.IGNORECASE)
FREERTOS_INCLUDE_PATTERN = re.compile(
    r"^(FreeRTOS\.h|FreeRTOSConfig\.h|task\.h|queue\.h|semphr\.h|timers\.h|event_groups\.h"
    r"|portmacro\.h|cmsis_os.*\.h)$"
)

INFRASTRUCTURE_LIBS = ("bsp", "os")
INFRASTRUCTURE_LAYERS = ("bsp", "os")
INTERFACE_HEADER_SUFFIX = "_requirements.hpp"
DATA_TYPES_HEADER_SUFFIX = "_types.hpp"
VIRTUAL_PATTERN = re.compile(r"\bvirtual\b")

# libs/AGENTS.md maps a library directory to a namespace scope; the two interface-only libs
# publish the namespace of the layer they describe rather than their own directory name.
LIBRARY_NAMESPACE_ALIASES = {"bsp-types": "bsp", "os-types": "os"}


@dataclass(frozen=True)
class Violation:
    check: str
    path: str
    line: int
    message: str

    def render(self) -> str:
        return f"{self.path}:{self.line}: [{self.check}] {self.message}"


@dataclass(frozen=True)
class SourceFile:
    path: Path
    relative_path: str
    text: str

    def includes(self) -> list[tuple[int, str]]:
        return [
            (self.text.count("\n", 0, match.start()) + 1, match.group(1))
            for match in INCLUDE_PATTERN.finditer(self.text)
        ]

    def namespaces(self) -> list[str]:
        return [match.group(1) for match in NAMESPACE_PATTERN.finditer(self.text)]


def CollectSourceFiles(repo_root: Path) -> list[SourceFile]:
    collected: dict[str, SourceFile] = {}
    for glob_pattern in SCANNED_GLOBS:
        for path in repo_root.glob(glob_pattern):
            if not path.is_file() or path.suffix not in SOURCE_SUFFIXES:
                continue
            relative_path = path.relative_to(repo_root).as_posix()
            if relative_path in collected:
                continue
            collected[relative_path] = SourceFile(
                path=path,
                relative_path=relative_path,
                text=path.read_text(encoding="utf-8", errors="replace"),
            )
    return [collected[key] for key in sorted(collected)]


def IsHalInclude(include: str) -> bool:
    return bool(HAL_INCLUDE_PATTERN.match(Path(include).name)) or include.startswith("stm32")


def IsFreertosInclude(include: str) -> bool:
    return bool(FREERTOS_INCLUDE_PATTERN.match(Path(include).name))


def IsInterfaceHeader(include: str) -> bool:
    return include.endswith(INTERFACE_HEADER_SUFFIX)


def IsDataTypesHeader(include: str) -> bool:
    return include.endswith(DATA_TYPES_HEADER_SUFFIX)


def FirmwareLayerOf(relative_path: str) -> tuple[str, str] | None:
    parts = relative_path.split("/")
    if len(parts) < 3 or parts[0] != "firmwares":
        return None
    return parts[1], parts[2]


def LibraryOf(relative_path: str) -> str | None:
    parts = relative_path.split("/")
    if len(parts) < 2 or parts[0] != "libs":
        return None
    return parts[1]


def IsCompositionRoot(relative_path: str) -> bool:
    return (
        "/app/src/composition/" in relative_path
        or "/app/include/app/composition/" in relative_path
        or relative_path.endswith("/app/src/application.cpp")
    )


def IsInfrastructureFile(relative_path: str) -> bool:
    firmware_layer = FirmwareLayerOf(relative_path)
    return (
        firmware_layer is not None and firmware_layer[1] in INFRASTRUCTURE_LAYERS
    ) or LibraryOf(relative_path) in INFRASTRUCTURE_LIBS


def CheckHalConfinement(source: SourceFile) -> list[Violation]:
    firmware_layer = FirmwareLayerOf(source.relative_path)
    library = LibraryOf(source.relative_path)
    hal_is_allowed = (
        (firmware_layer is not None and firmware_layer[1] in ("bsp", "os"))
        or library in INFRASTRUCTURE_LIBS
        or IsCompositionRoot(source.relative_path)
    )
    if hal_is_allowed:
        return []
    return [
        Violation(
            check="hal-outside-bsp",
            path=source.relative_path,
            line=line,
            message=(
                f'includes the HAL/CMSIS header "{include}" outside bsp/, os/ and the '
                "composition root (firmwares/AGENTS.md F.3.2)"
            ),
        )
        for line, include in source.includes()
        if IsHalInclude(include)
    ]


def CheckFreertosConfinement(source: SourceFile) -> list[Violation]:
    firmware_layer = FirmwareLayerOf(source.relative_path)
    freertos_is_allowed = (
        firmware_layer is not None and firmware_layer[1] == "os"
    ) or LibraryOf(source.relative_path) == "os"
    if freertos_is_allowed:
        return []
    return [
        Violation(
            check="freertos-outside-os",
            path=source.relative_path,
            line=line,
            message=(
                f'includes the FreeRTOS header "{include}" outside the os layer; use the os '
                "wrappers (firmwares/AGENTS.md F.3.3)"
            ),
        )
        for line, include in source.includes()
        if IsFreertosInclude(include)
    ]


def CheckDomainPurity(source: SourceFile) -> list[Violation]:
    library = LibraryOf(source.relative_path)
    firmware_layer = FirmwareLayerOf(source.relative_path)
    is_domain_code = (library is not None and library not in INFRASTRUCTURE_LIBS) or (
        firmware_layer is not None and firmware_layer[1] == "domain"
    )
    if not is_domain_code:
        return []

    violations = []
    for line, include in source.includes():
        if IsHalInclude(include) or IsFreertosInclude(include):
            continue
        if include.startswith(("bsp/", "os/")):
            violations.append(
                Violation(
                    check="domain-purity",
                    path=source.relative_path,
                    line=line,
                    message=(
                        f'includes the infrastructure header "{include}"; domain code takes its '
                        "interfaces and types from the bsp-types and os-types packages "
                        "(AGENTS.md 2.3, libs/AGENTS.md L.1)"
                    ),
                )
            )
    return violations


def CheckCompositionRootConfinement(source: SourceFile) -> list[Violation]:
    firmware_layer = FirmwareLayerOf(source.relative_path)
    if firmware_layer is None or firmware_layer[1] != "app":
        return []
    if IsCompositionRoot(source.relative_path):
        return []
    return [
        Violation(
            check="concrete-bsp-outside-composition",
            path=source.relative_path,
            line=line,
            message=(
                f'includes the concrete BSP header "{include}"; app code depends on '
                "*_requirements.hpp interfaces and *_types.hpp data headers, concrete BSP "
                "belongs to the composition root (firmwares/AGENTS.md F.3.1, F.4)"
            ),
        )
        for line, include in source.includes()
        if include.startswith("bsp/")
        and not IsInterfaceHeader(include)
        and not IsDataTypesHeader(include)
    ]


def CheckDataTypesHeaderPurity(source: SourceFile) -> list[Violation]:
    if not IsDataTypesHeader(source.relative_path) or not IsInfrastructureFile(
        source.relative_path
    ):
        return []

    violations = [
        Violation(
            check="types-header-purity",
            path=source.relative_path,
            line=line,
            message=(
                f'includes "{include}"; a *_types.hpp header is the data contract app and domain '
                "code may include, so it stays free of HAL and FreeRTOS "
                "(firmwares/AGENTS.md F.3.2)"
            ),
        )
        for line, include in source.includes()
        if IsHalInclude(include) or IsFreertosInclude(include)
    ]

    virtual_match = VIRTUAL_PATTERN.search(source.text)
    if virtual_match is not None:
        violations.append(
            Violation(
                check="types-header-purity",
                path=source.relative_path,
                line=source.text.count("\n", 0, virtual_match.start()) + 1,
                message=(
                    "declares a virtual member in a *_types.hpp header; polymorphism belongs to "
                    "a *_requirements.hpp interface (firmwares/AGENTS.md F.3.2)"
                ),
            )
        )
    return violations


def ExpectedNamespaceOf(relative_path: str) -> str | None:
    parts = relative_path.split("/")
    library = LibraryOf(relative_path)
    if library is not None:
        scope = LIBRARY_NAMESPACE_ALIASES.get(library, library).replace("-", "_")
        if len(parts) >= 4 and parts[2] == "include":
            sub_domains = parts[4:-1]
        elif len(parts) >= 3 and parts[2] == "src":
            sub_domains = parts[3:-1]
        else:
            return None
        return "::".join(["midismith", scope, *sub_domains])

    firmware_layer = FirmwareLayerOf(relative_path)
    if firmware_layer is None:
        return None
    firmware, layer = firmware_layer
    if layer not in ("app", "bsp", "os", "domain"):
        return None
    scope = firmware.replace("-", "_")
    if len(parts) >= 5 and parts[3] == "include":
        sub_domains = parts[5:-1]
    elif len(parts) >= 4 and parts[3] == "src":
        sub_domains = parts[4:-1]
        if sub_domains and sub_domains[0] == layer:
            sub_domains = sub_domains[1:]
    else:
        return None
    return "::".join(["midismith", scope, layer, *sub_domains])


def CheckNamespaceMirrorsDirectory(source: SourceFile) -> list[Violation]:
    expected_namespace = ExpectedNamespaceOf(source.relative_path)
    if expected_namespace is None:
        return []
    declared_namespaces = source.namespaces()
    if not declared_namespaces:
        return []
    if any(
        declared == expected_namespace or declared.startswith(f"{expected_namespace}::")
        for declared in declared_namespaces
    ):
        return []
    return [
        Violation(
            check="namespace-path-mismatch",
            path=source.relative_path,
            line=1,
            message=(
                f"declares {', '.join(declared_namespaces)} but its directory maps to "
                f"{expected_namespace} (AGENTS.md namespace hierarchy)"
            ),
        )
    ]


def CheckPackageTestContract(repo_root: Path) -> list[Violation]:
    violations = []
    for library_path in sorted((repo_root / "libs").glob("*")):
        if not library_path.is_dir() or library_path.name in INFRASTRUCTURE_LIBS:
            continue
        if not (library_path / "src").is_dir():
            continue
        if (library_path / "tests").is_dir():
            continue
        relative_path = library_path.relative_to(repo_root).as_posix()
        violations.append(
            Violation(
                check="missing-tests-directory",
                path=relative_path,
                line=1,
                message="has src/ but no tests/; every domain library owns its tests (AGENTS.md 7)",
            )
        )
    return violations


PER_FILE_CHECKS = (
    CheckHalConfinement,
    CheckFreertosConfinement,
    CheckDomainPurity,
    CheckCompositionRootConfinement,
    CheckDataTypesHeaderPurity,
    CheckNamespaceMirrorsDirectory,
)


def Main() -> int:
    parser = argparse.ArgumentParser(description="Check monorepo architecture rules.")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parent.parent)
    arguments = parser.parse_args()
    repo_root = arguments.repo_root.resolve()

    sources = CollectSourceFiles(repo_root)
    violations = []
    for source in sources:
        for check in PER_FILE_CHECKS:
            violations.extend(check(source))
    violations.extend(CheckPackageTestContract(repo_root))

    for violation in sorted(violations, key=lambda item: (item.path, item.line)):
        print(violation.render())

    if violations:
        print(f"\nArchitecture check: {len(violations)} violation(s) over {len(sources)} files.")
        return 1
    print(f"Architecture check: clean over {len(sources)} files.")
    return 0


if __name__ == "__main__":
    sys.exit(Main())
