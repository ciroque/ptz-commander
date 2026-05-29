# PTZ Commander — Refactoring & Code Improvement Opportunities

This document captures the highest-value refactorings and structural improvements identified in the current codebase (as of the `feat/named-preset-files` branch).

The goal is to improve maintainability, reduce duplication, make future features easier to add, and reduce the risk of bugs (such as the recent Tail Air pan/tilt reversal).

Items are roughly prioritized by **impact vs effort**.

---

## Recently Completed

### Eliminate `splitArgs()` Duplication (Original Item #1)
- Created a shared implementation in the new file [commands/Arguments.h](/commands/Arguments.h) (`commands::splitArgs()`).
- Removed the duplicated private `splitArgs()` method from ~12 command headers (`preset/*`, `camera/*`, `snapshot/*`) and `MoveCommand.cpp`.
- Updated all call sites to use the centralized function.
- Added the new header to `CMakeLists.txt`.
- This was the highest-impact quick win in the original list.

### Help Command Review & Improvements
- Performed a full review of **all** help commands:
  - Root `help`
  - `camera help`
  - `preset help`
  - `snapshot help`
- Fixed inaccurate command syntax in `camera help` (especially `move <id> <x> <y> <z>` and the incomplete `push` entry).
- Improved consistency of argument style (`<id|*>`, `<name>`, etc.) and descriptions across all help output.
- Added missing `cls` command to the root help.
- Fixed incorrect internal usage string in `PushCommand.cpp` (was printing `push ...` instead of `camera push ...`).
- This work partially addresses the "Clean up the multiple HelpCommand classes" item that was previously in the Lower Priority section.

---

## High Impact / High ROI (Strongly Recommended)

### ~~1. Eliminate `splitArgs()` Duplication~~ (COMPLETED)

**Status**: Completed during work on the `feat/named-preset-files` branch.  
See the **Recently Completed** section at the top of this document for details.

---

### 1. Complete the Control Strategy Pattern (Reading Side)

**Problem**: We have a good `ObsbotControlStrategy` for *writing* PTZ (move/zoom), but `getCurrentPtz()` lives only in `ObsbotCamera` with a single code path using `gimbalGetAttitudeInfoR`.

The recent pan/tilt bug showed that different camera families can have different coordinate or data ordering behaviors.

**Impact**: High (prevents future coordinate/reading bugs)
**Effort**: Medium
**Suggested approach**:
- Add `getCurrentPtz(Device* dev)` (or similar) to `ObsbotControlStrategy`.
- Move the current reading logic into `TinyFamilyStrategy` and create appropriate logic for `TailAirStrategy`.
- This makes the Strategy pattern actually own the full PTZ contract.

---

### 2. Extract Duplicate Logic from Camera Strategies

**Problem**: `TailAirStrategy` and `TinyFamilyStrategy` duplicate:
- The `clamp` template function
- Zoom scaling logic (`scaledZoom`, `zoomRatio`, speed calculation)

There is also an unused `scaleToRange()` function in `cameras/utils.h`.

**Impact**: Medium–High
**Effort**: Low
**Suggested approach**:
- Move shared math into `cameras/obsbot/strategies/` (e.g. `StrategyUtils.h` or a base class with protected helpers).
- Either delete or integrate the existing `utils.h` function.

---

### 3. Improve CMake Build Configuration

**Problem**: `CMakeLists.txt` manually lists every single `.cpp` and `.h` file. This is brittle, error-prone, and painful when adding new commands or cameras.

**Impact**: High for long-term maintainability
**Effort**: Low–Medium
**Suggested approach**:
- Use `file(GLOB_RECURSE ...)` (with care) **or** better: organize into proper targets and use `target_sources()`.
- Consider splitting into subdirectories with their own `CMakeLists.txt` includes.
- At minimum, group files logically instead of a giant flat list.

---

## Medium Impact / Good Architecture Improvements

### 4. Introduce a Proper Preset Persistence Abstraction

**Context**: The current `feat/named-preset-files` work is adding filename support to `LoadCommand`/`SaveCommand`, but persistence logic is still scattered directly in the command classes using raw `nlohmann::json` + `std::fstream`.

**Impact**: High (especially as named preset files mature)
**Effort**: Medium
**Suggested approach**:
- Create a `PresetStore` / `PresetRepository` class (or interface + implementations).
- Move JSON serialization, file handling, and the default filename logic there.
- This will make future features (auto-save, multiple backends, validation, migration from `.json` → `.ptzc`) much cleaner.

---

### 5. Reduce Raw `std::cout` Usage in Commands

**Problem**: Almost all user-facing output is done via raw `std::cout` directly inside command implementations. `core::Logger` (spdlog) is only used in the camera adapter layer.

**Impact**: Medium (consistency + future testability / redirection)
**Effort**: Medium
**Options**:
- Introduce a simple `Output` / `Console` abstraction passed via `Context`.
- Or make Logger more central for *all* output (with different levels/sinks).
- At minimum, centralize common output patterns (success messages, tables, errors).

---

### 6. Improve Command Registration and Dispatch

**Current state**: `CommandHandler` constructor manually `addCommand()`s every single command. Very repetitive.

**Impact**: Medium
**Effort**: Low–Medium
**Ideas**:
- Use a registration macro or a static registry.
- Or use a simple dependency injection / service locator for commands.
- Consider moving toward a more data-driven command table.

---

### 7. Better Handling of Preset vs Snapshot Concepts

Currently there are two overlapping concepts:
- Per-camera named presets (`preset store/apply/discard`)
- Cross-camera "snapshots" (`snapshot apply`)

The relationship between them is not very clear in the code or architecture.

**Impact**: Medium (clarity + future features)
**Effort**: Medium

---

## Lower Priority / Nice-to-Have

- Add basic unit test infrastructure (even a minimal Catch2 or doctest setup would help).
- Create a small, reusable argument parsing library (support for flags, quoted strings, typed parsing).
- ~~Clean up the multiple `HelpCommand` classes (root + per domain)~~ — partially addressed (see Recently Completed section).
- Consider extracting common camera lookup logic (`*` vs specific ID) into a helper.
- Review and potentially remove or properly integrate `cameras/utils.h`.
- Add a `.clang-format` / consistent code style (indentation, bracing, etc. are currently inconsistent between files).
- Document the coordinate system assumptions per camera family (especially after the recent Tail Air fix).

---

## Notes

- Many of these items would make excellent **small, focused PRs** — exactly the style you mentioned wanting to maintain.
- The Strategy pattern completion for reading (#1) and persistence abstraction (#4) would probably give high value for future work.
- The persistence abstraction (#5) becomes increasingly valuable as the named preset files feature grows.

---

**Last updated**: After completing the `splitArgs()` deduplication and a full review + standardization of all in-app help commands (on the `feat/named-preset-files` branch).

Feel free to edit, prioritize, or turn items into GitHub issues / individual PR plans.
