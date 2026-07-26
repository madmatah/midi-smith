---
name: writing-commits
description: Writing git commit messages for this project: the gitmoji header format, how to pick the emoji and the scope, and how to explain the change in the body. Use when committing, amending, rewording, splitting a change into commits, or reviewing a branch's history.
user-invocable: true
---

# Writing Commits

## The format

Adapted from the Angular guidelines, with the type replaced by a gitmoji:

```
<emoji>(<scope>) <subject>
<blank line>
<body>
<blank line>
<footer>
```

The header is the one-line summary of the change. The body is where you explain why you made it.
The footer carries breaking changes and issue references. Only the header is always present.

```
🐛(main-board) let the confirmation reach the console before the reset

The `firmware update` command answered on the shell and reset the board
in the same breath. The transmitter was still draining when the reset
hit, so the operator saw a truncated line, or nothing at all, and had no
way to tell a staged image from a refused one.

Waiting for the transmitter costs a few milliseconds on a path that
reboots anyway, and it is the last moment where the operator can still
read what the board decided.
```

- The emoji is the unicode character, never the shortcode: `✨`, not `:sparkles:`.
- No colon anywhere in the header; a single space separates `(scope)` from the subject.
- No em dash, in the header or in the body. A message is not a file, so `prose_check` cannot see
  it; the ban of `AGENTS.md` applies all the same. Use a semicolon, a colon, parentheses, or a
  second sentence.
- Never a `Co-Authored-By`, a `Generated with` line, or a `Signed-off-by`.

## Granularity

A commit is one reason to change, not one work session. A feature branch carrying several commits
means the feature needed several distinct decisions, and each one is readable on its own.

- Every commit in the branch must be green on its own: the test suite passes at each point of the
  history, not only at its tip.
- Finish and verify one item, commit it, then start the next. Do not batch unrelated fixes into a
  single commit because they happened on the same afternoon.
- Fixing something a previous commit of the same branch got wrong is a `git commit --fixup` plus a
  `git rebase --autosquash`, not a follow-up commit. The history describes the change that shipped,
  not the path taken to find it.

## Scope

The scope names the package or the area the change belongs to, in lowercase, exactly as its
directory is named:

| The change lands in | Scope |
|---|---|
| `firmwares/main-board/`, `firmwares/adc-board/`, `firmwares/bootloader/` | `main-board`, `adc-board`, `bootloader` |
| a library under `libs/` | the library's directory name: `dsp`, `midi`, `boot-control`, `bsp-flash`, `update-catalogue` |
| `CMakeLists.txt`, presets, the flash map, packaging | `build` |
| `.github/workflows/` | `ci` |
| `tools/` | `tools` |
| documentation belonging to no single package | `docs` |
| `AGENTS.md` and the rules it carries | `agents` |
| `.agents/skills/`, `.claude/agents/` | the skill or gate it changes: `qa`, `skills` |
| the repository itself (`.gitignore`, license, editor configuration) | `project` |

A hardware or CubeMX guide under `firmwares/<name>/docs/` belongs to its firmware, so it carries
the firmware's scope, not `docs`.

Rules: never a file name, never a path, never capitalized, never two scopes at once. A change that
genuinely spans two packages is usually two commits; when it truly cannot be split, use the scope of
the package that owns the decision.

## Subject

The subject says what the change achieves, in the reader's terms. `git diff` already lists what was
touched, and repeating it wastes the line that decides whether anyone opens the commit at all.

- Imperative present tense: `add`, never `added` or `adds`.
- Lowercase throughout, acronyms and product names included: `cubemx`, `crc-32`, `dma`, `bdd`,
  `msfw`, `h743`. A symbol or file name that must appear keeps its real casing.
- No trailing period.
- Around 70 characters is comfortable; stop where the sentence stops reading well. What does not
  fit belongs in the body, never in a longer header.

💩 States the mechanism, or nothing at all:

```
🐛(main-board) fix sd card bug
🐛(main-board) add a delay before HAL_NVIC_SystemReset
✨(main-board) update shell.cpp and sd_reader.cpp
🐛(main-board) Fixed the mount error.
```

❤️ States the intent:

```
🐛(main-board) let the confirmation reach the console before the reset
🐛(main-board) say why a mount failed instead of trusting a detect contact
🐛(update-catalogue) never call a board up to date when its version is unknown
♻️(firmware-installer) name the lib after the firmware family it belongs to
```

## Body

The body is where you explain yourself. Write it in the same imperative present tense as the
subject, wrapped around 72 columns.

Include one on every commit whose reason is not obvious from the subject, which is most of them.
Skip it only when the change genuinely speaks for itself: a typo, a mechanical rename, a dependency
bump.

The history predating this skill is header-only. Do not read the convention off `git log`: those
commits are what this rule exists to stop repeating.

- Give the **motivation**, and contrast it with the behavior that was there before. What broke,
  what constraint appeared, what the previous code assumed and when that assumption stopped holding.
- Do **not** list what you changed. `git diff` does that better than prose, and never goes stale.
- Say what you decided **against** when the alternative was tempting. A rejected approach is often
  the most useful thing a body can carry, because nothing else in the repository records it.
- On this project, name the physical reality behind the change when there is one: the datasheet
  line, the socket wiring, the measured stack peak, the DMA alignment constraint. That is knowledge
  no reader can recover from the code.

See the body as a brain dump of your state of mind at the moment you commit. Imagine someone
irritated chasing you six months from now:

> Why on earth did you do this ??

You would not answer with a list of the files you edited. You would explain and justify the context
that led you there, the context you will have forgotten by then. Without it your change gets
misread and criticized, or worse, reverted by someone who could not see what it was protecting
against. That someone is, most likely, the future you.

Under `🐛(main-board) read the detect switch the way this socket actually wires it`:

💩 A list of the diff, which the diff already gives:

```
Invert the card detect GPIO read and change the pull configuration to
pull-up. Update the mount test accordingly.
```

❤️ The context that led there:

```
The socket fitted on this board grounds the detect contact when a card
is inserted, the opposite polarity from the one the driver assumed. An
empty slot read as occupied, so every boot without a card walked into a
mount that could only fail, and reported a file system error for what
was really an empty tray.

Reading the contact as active-low with the internal pull-up matches the
schematic and lets the slot answer honestly before the file system is
ever asked anything.
```

## Footer

Reserved for the two things that have to survive outside prose:

- A **breaking change**, starting with `BREAKING CHANGE:`, covering a wire format, a container
  layout, a flash map, a `*Requirements` signature every implementation now has to follow. Say
  what breaks and what a consumer must do about it.
- The **issues** this commit closes: `Closes #42`.

Never invent either one.

## Choosing the emoji

Pick the single emoji matching the dominant intent of the change. When two seem to fit, these are
the pairs that actually collide in this repository:

| Situation | Emoji | Not |
|---|---|---|
| A defect in shipped behavior | 🐛 | 🩹, which is a small blemish nothing depended on |
| A defect that breaks the board or the build right now | 🚑️ | 🐛 |
| New observable behavior | ✨ | 👔, unused here; domain logic arrives as a feature |
| Restructuring with no behavior change | ♻️ | 🎨, which is formatting and layout only |
| Anything the user sees on the TFT | 💄 | 🎨 |
| A package boundary, a layer, a new lib | 🏗️ | ♻️ |
| CMake, presets, flash map, packaging | 👷 | 🔧 |
| CI workflows | 👷 | 💚, which is only for repairing a red CI |
| A CubeMX regeneration | 👷 | 🔧 |
| Tool, editor, agent or repository configuration | 🔧 | 👷 |
| A script under `tools/` you run by hand | 🔨 | 👷, once the build or the CI runs it |
| Documentation, guides, `AGENTS.md` | 📝 | 📄, which is the license file only |
| Tests added or extended | ✅ | 🧪, which is a deliberately failing test |
| Deleting code that still had callers | 🔥 | ⚰️, which is code already unreachable |
| Undoing an earlier decision | ⏪️ | 🔥 |

Two entries of the catalogue cannot legitimately appear here: 💡 (comments in source code) is
forbidden by the zero-comment policy, and 🍻 speaks for itself.

## The catalogue

This list is the complete set. An emoji outside it is not a valid type.

| Emoji | Use for |
|---|---|
| 🎨 | Improving the structure or format of the code |
| ⚡️ | Improving performance |
| 🔥 | Removing code or files |
| 🐛 | Fixing a bug |
| 🚑️ | Applying a critical hotfix |
| ✨ | Introducing a feature |
| 📝 | Adding or updating documentation |
| 🚀 | Deploying |
| 💄 | Adding or updating the user interface and styles |
| 🎉 | Beginning a project |
| ✅ | Adding, updating or passing tests |
| 🔒️ | Fixing a security or privacy issue |
| 🔐 | Adding or updating secrets |
| 🔖 | Releasing or tagging a version |
| 🚨 | Fixing compiler or linter warnings |
| 🚧 | Marking work in progress |
| 💚 | Fixing the CI build |
| ⬇️ | Downgrading dependencies |
| ⬆️ | Upgrading dependencies |
| 📌 | Pinning dependencies to specific versions |
| 👷 | Adding or updating the CI or build system |
| 📈 | Adding or updating analytics or tracking |
| ♻️ | Refactoring code |
| ➕ | Adding a dependency |
| ➖ | Removing a dependency |
| 🔧 | Adding or updating configuration files |
| 🔨 | Adding or updating development scripts |
| 🌐 | Internationalizing or localizing |
| ✏️ | Fixing typos |
| 💩 | Writing deliberately bad code to be improved later |
| ⏪️ | Reverting changes |
| 🔀 | Merging branches |
| 📦️ | Adding or updating compiled files or packages |
| 👽️ | Adapting code to a changed external API |
| 🚚 | Moving or renaming resources |
| 📄 | Adding or updating the license |
| 💥 | Introducing a breaking change |
| 🍱 | Adding or updating assets |
| ♿️ | Improving accessibility |
| 💡 | Adding or updating comments in source code |
| 🍻 | Writing code drunk |
| 💬 | Adding or updating texts and literals |
| 🗃️ | Changing the database |
| 🔊 | Adding or updating logs |
| 🔇 | Removing logs |
| 👥 | Adding or updating contributors |
| 🚸 | Improving user experience or usability |
| 🏗️ | Changing the architecture |
| 📱 | Working on responsive design |
| 🤡 | Adding or updating mocks |
| 🥚 | Adding or updating an easter egg |
| 🙈 | Adding or updating `.gitignore` |
| 📸 | Adding or updating snapshots |
| ⚗️ | Running an experiment |
| 🔍️ | Improving SEO |
| 🏷️ | Adding or updating types |
| 🌱 | Adding or updating seed data |
| 🚩 | Adding, changing or removing feature flags |
| 🥅 | Catching errors |
| 💫 | Adding or updating animations and transitions |
| 🗑️ | Deprecating code to be cleaned up |
| 🛂 | Changing authorization, roles or permissions |
| 🩹 | Applying a simple, non-critical fix |
| 🧐 | Exploring or inspecting data |
| ⚰️ | Removing dead code |
| 🧪 | Adding a failing test |
| 👔 | Adding or updating business logic |
| 🩺 | Adding or updating a healthcheck |
| 🧱 | Changing infrastructure |
| 🧑‍💻 | Improving the developer experience |
| 💸 | Adding sponsoring or financial infrastructure |
| 🧵 | Changing multithreading or concurrency |
| 🦺 | Adding or updating validation |
| ✈️ | Improving offline support |
| 🦖 | Adding backwards compatibility |

## Before you commit

1. A header of exactly one line: `<emoji>(<scope>) <subject>`, no colon after the scope.
2. An emoji from the catalogue, matching the dominant intent.
3. A lowercase scope naming a package or area, in parentheses, followed by one space.
4. An imperative, lowercase, period-free subject stating the effect, not the files.
5. A body, after a blank line, explaining why, unless the change truly speaks for itself.
6. The body gives the motivation and the previous behavior, not a summary of the diff.
7. A footer only for a breaking change or a closed issue. No trailer, ever.
8. The tree is green at this commit, not just at the tip of the branch.
