---
name: qa
description: Runs the end-of-contribution QA gate over the current change — the deterministic floor (format, lint, architecture, host tests, both firmware builds) then the specialist reviewers the diff calls for. Use before a change is called done.
user-invocable: true
---

# QA Gate

You are running the project's end-of-contribution QA gate. Do this now, before the change is
called done. An argument (a feature name, a path, a commit range) narrows the scope; without one,
the scope is the current change.

## 1. Scope the change

```sh
git diff --name-only                                          # uncommitted work
git diff --name-only "$(git merge-base HEAD origin/main)"..HEAD   # committed work on a branch
```

Use both when both are non-empty. If the diff is documentation only, output
**"QA gate: out of scope (documentation only)."** and stop.

## 2. Run the deterministic floor FIRST

Never pay a reviewer to find what a tool already reports, and never spend reviewers on a change
that does not build.

```sh
cmake --preset Host-Debug
cmake --build --preset Host-Debug --target format_check
cmake --build --preset Host-Debug --target lint
cmake --build --preset Host-Debug --target architecture_check
bash .agents/skills/running-tests/scripts/run_all_host_tests.sh
```

Then both firmware builds, which are the only check that the cross-compiled targets still link and
still fit:

```sh
cmake --preset adc-Release  && cmake --build --preset adc-Release
cmake --preset main-Release && cmake --build --preset main-Release
```

Run the firmware builds AFTER the host tests: they switch the active preset. Report every red
result, then sort it: a build or test failure blocks the review (do not dispatch reviewers over a
broken tree), while a format failure is mechanical — fix it and carry on in the same pass.

Fixing each kind of red:

- **`format_check`** — run `cmake --build --preset Host-Debug --target format`. clang-format is
  idempotent, so it rewrites exactly the files `format_check` just named and leaves the rest
  untouched. Then read `git status`: on a branch off a green `main` the rewritten files are your
  own, but any file this change never touched is pre-existing drift (usually a local clang-format
  older or newer than the CI container). Do not fold that into the branch silently — report it and
  let the user decide whether it belongs here or in its own commit.
- **`lint`** — cpplint has no auto-fix; each violation is edited by hand. A line over 100 columns
  is usually clang-format's job, so run the format fix first and re-check.
- **`architecture_check`** — cites the rule it breaks. The fix goes in the code. The guard carries
  no per-file allowlist, so the only other legitimate outcome is amending the RULE — a named
  constant in the check plus its line in the owning AGENTS.md — and that is the user's call, not
  yours (`AGENTS.md` 5).

## 3. Dispatch the reviewers the diff calls for

Spawn them FRESH and in PARALLEL (one message, several subagents). Never let the code's author
review its own work in the same context — that is the whole point of the fan-out.

**Size the change before you size the fan-out.** Measure the C++ delta only, not the diff as a
whole — tooling, markdown and generated files inflate it and are nobody's review surface:

```sh
git diff --shortstat origin/main...HEAD -- 'firmwares/*.cpp' 'firmwares/*.hpp' 'libs/*.cpp' 'libs/*.hpp'
```

The table below says WHICH dimension a surface calls for; the size says HOW MANY reviewers are
worth paying for:

- **Under ~100 changed C++ lines, or a mechanical rename**: ONE reviewer, the dimension the change
  is actually about. Four reviewers over a small refactor costs more than reading it yourself.
- **Up to ~500 lines**: the one or two rows that match most directly.
- **A completed deliverable set, or new logic across several packages**: every matching row.

| The diff touches | Dispatch |
|---|---|
| a `libs/` domain library | `conventions-reviewer`, `test-coverage-auditor` |
| `firmwares/*/domain/` | `conventions-reviewer`, `test-coverage-auditor` |
| `firmwares/*/app/` tasks, messaging, storage, UI | `architecture-reviewer`, `test-coverage-auditor` |
| `firmwares/*/app/src/composition/`, a new `*Requirements`, a new lib, a CMake dependency edge | `architecture-reviewer` |
| `firmwares/*/bsp/`, `firmwares/*/os/`, `libs/bsp`, `libs/os` | `embedded-safety-reviewer`, `architecture-reviewer` |
| a fixed buffer, a formatter, an ISR, a shared or static object, a DMA path, a per-sample or per-tick path | `embedded-safety-reviewer` |
| any new or renamed public symbol, file or directory | `conventions-reviewer` |
| tests only | `test-coverage-auditor` |

Write each dispatch prompt to protect the reviewer's budget, because a reviewer that runs out of
turns delivers NOTHING — not a partial report, nothing:

- name the exact files in scope and say what is out of scope, so it does not re-derive the perimeter;
- state its turn budget explicitly and tell it to start with one `git diff` over the scope rather
  than opening files one by one;
- state the questions this change actually raises, not just "review it": a reviewer given a thesis
  to test converges, a reviewer given a file list explores.

## 4. Check the CubeMX contract yourself (no agent)

Only when the diff touches `Core/`, `Drivers/`, `Middlewares/`, `USB_DEVICE/` or a `.ioc` file —
four items, `firmwares/AGENTS.md` F.1 and F.7:

- `Drivers/` is unchanged (it is strictly read-only).
- Every edit to a generated file sits inside a `USER CODE` zone.
- A `.ioc` change is documented in `firmwares/<firmware>/docs/stm32cubemx-configuration-guide.md`.
- The change was applied by the operator in CubeMX, not hand-written into generated code. If it
  was not, stop and ask for it to be done in CubeMX and regenerated.

## 5. Confirm before acting

Reviewers report COVERAGE, not verdicts: they are told not to filter. Roughly half of raw findings
do not survive a second look, so verify each consequential finding against the code yourself before
you act on it. Discard the ones that do not hold, and say how many you discarded.

## 6. Fix and report

Fix every BLOCKING and SHOULD-FIX finding that survived. One commit per finding, in the project's
one-line gitmoji format. Re-run the affected part of the floor after the fixes.

End with:

```
## QA Gate: <change>

**Scope:** <files, N across which packages>
**Floor:** format_check / lint / architecture_check / host tests / adc-Release / main-Release
**Reviewers:** <which, and why>

### Fixed
- <finding> -> <commit>

### Remaining
- [VERIFY] <needs the hardware, a scope capture, or the CI bloaty report>
- [NOTE] <deferred, with the reason>

### Verdict
READY | NOT READY
```

READY is advisory judgement. Anything that cannot be settled from code and host tests — real-time
behavior on the board, MIDI latency, flash/RAM impact, a peripheral bring-up — is a `[VERIFY]`
item for the maintainer, never a `[PASS]`.
