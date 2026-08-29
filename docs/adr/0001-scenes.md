# ADR 0001: Scenes as named preset bindings

- **Status:** Proposed
- **Date:** 2026-08-28
- **Deciders:** project maintainer + implementation notes from preset-file work
- **Related:** `REFACTORING.md` item 7 (preset vs snapshot); `.ptzc` load/save on `PresetStore`

## Context

A **preset** is a named PTZ snapshot on one camera (`preset store` / `apply` / `discard`). That model is correct and stays.

A **snapshot** (`snapshot apply <name>`) is not a second store. It means “apply the *same preset name* on every camera that has it.” In practice that forces you to name unrelated looks alike (`pulpit` on three cameras that are not doing the same thing) just to get a one-word cue. That is the grouping mechanism we have, and it is the wrong one.

A **scene** is the grouping we actually want: a named set of *which preset on which camera*, applied as one cue.

```
sermon:
  house-left   → close
  house-center → pulpit
  house-right  → wide
```

`scene apply sermon` instead of “hope everyone stored something named sermon.”

This ADR is only about scenes inside ptz-commander. OBS (`ProgramSceneChanged` → `scene apply`) is a later client of this contract, not part of this decision.

## Decision

1. A scene is a **name plus bindings**: camera serial → preset name.
2. Bindings are **references**, resolved at apply time. They do not bake pan/tilt/zoom.
3. Scenes live **in the same `.ptzc` file** as presets, under a reserved top-level key `"scenes"`. They load and save with `preset load` / `preset save` (replace, same as presets).
4. In memory, scenes live in a **`SceneStore` independent of `Camera` objects**, so a scene still exists if a camera is unplugged.
5. Composition is explicit (`scene add`), not capture (`scene store` of “whatever I’m on now”).
6. `snapshot *` is left in place for now and is not extended. Scenes are the real cross-camera cue.

## Data model

```text
Scene {
  name: string                          // "sermon"
  bindings: map<serial, presetName>     // "RMOWTHF7211JGR" → "close"
}
```

- **Persist serials**, not aliases. Aliases change; serials are the `Camera` identity (`findById` already matches alias / serial / name for *input*).
- Display uses alias when set (`getFriendlyName()`), same as `preset list`.
- VISCA ids (`VISCA:COM3:1`) are still port-shaped. That is an existing identity problem, not a scene-specific one. Document it; do not invent a second id scheme here.

`SceneStore` owns `map<name, Scene>`. It does **not** hang off `Camera`. Presets stay on `Camera`; scenes point at them.

`data::Context` (or `Application` via Context) gains a `SceneStore&` so commands can see both cameras and scenes. `PresetStore::load` / `save` take the scene store as well as `CameraManager`.

## Persistence

Today a `.ptzc` file is a JSON object whose keys are camera serials. `load` only *reads* keys that match connected cameras, so a `"scenes"` sibling is currently ignored. `save` rewrites the whole file from connected cameras only, so a `"scenes"` sibling would be **destroyed** unless save learns about it.

Chosen on-disk shape (v1 files remain valid):

```json
{
  "RMOWTHF7211JGR": {
    "alias": "house-left",
    "presets": {
      "close": { "name": "close", "pan": 12.5, "tilt": -3.2, "zoom": 65 }
    }
  },
  "scenes": {
    "sermon": {
      "RMOWTHF7211JGR": "close",
      "BMOSZHHB071KGA": "pulpit"
    }
  }
}
```

Rules:

- `"scenes"` is a **reserved top-level key**. It is never treated as a camera serial (a camera actually named `scenes` would already be a problem; we do not support that).
- Scene objects map serial → preset name (strings). No nested PTZ blobs.
- `preset load` **replaces** in-memory scenes the same way it replaces presets. A file with no `"scenes"` key means “this collection has no scenes” (clear `SceneStore`). A parse error still leaves RAM unchanged (same all-or-nothing rule as presets).
- `preset save` writes `"scenes"` whenever the store is non-empty. Empty store: omit the key so files that never used scenes stay serial-only.
- Scene bindings for serials that are not currently connected are **kept** in `SceneStore` and round-trip through save. That is the opposite of presets, which can only attach to a live `Camera`.

No sidecar `.ptzs` files. They would drift from the presets they name.

## Commands

New `scene` family, same verb register as presets:

| Command | Meaning |
|---|---|
| `scene list` | Scene names (and binding counts) |
| `scene show <name>` | Bindings, alias + serial + preset |
| `scene add <name> <id> <preset> [<id> <preset> ...]` | Upsert bindings (positional pairs); create the scene if needed |
| `scene remove <name> <id>` | Drop one camera from the scene |
| `scene discard <name>` | Delete the scene |
| `scene apply <name>` | Resolve bindings and `setPosition` |
| `scene help` | Help |

Notes:

- `<id>` is the same as everywhere else: alias, serial, or product name via `findById`. The stored binding is always the serial.
- There is no `scene store`. Cameras do not know “which preset they are on”; inventing capture would mean fuzzy PTZ matching or last-apply tracking. Composition is the feature.
- `scene add` on an unknown preset name is an error (fail that binding, do not record a dangling name). Unknown camera id is an error.
- Persistence is still `preset save` / `load` / `browse`. Scenes are part of the collection, not a second file type.

## Apply semantics

`scene apply <name>`:

1. Look up the scene in `SceneStore`. Missing scene → error, no motion.
2. For each binding (in deterministic order by serial):
   - Resolve serial → live `Camera`. Missing camera → warn and skip (show continues).
   - `GetPresetByName`. Missing preset → warn and skip.
   - `setPosition(pan, tilt, zoom)` immediately. No inter-camera sleep (`preset apply *` delay is for cycling one camera’s presets, not a cue).
3. Report applied / skipped counts.

Reference resolution is **at apply time**. Edit `house-left`’s `close` PTZ, then `scene apply sermon` uses the new numbers. That is the point of not baking.

## Snapshots

`snapshot apply` / `snapshot list` stay as the old “same name on all cameras” shortcut. Do not add snapshot store, files, or help that competes with scenes.

A later cleanup (not this work) can make `snapshot apply X` sugar for “apply preset X on every camera that has it,” or remove the family once scenes cover the real workflow.

## Non-goals

- Baked PTZ inside a scene (frozen numbers independent of named presets). Could be a later `scene snapshot` / bake flag; not v1.
- Capturing “current look” into a scene.
- OBS plugin, obs-websocket, or mapping OBS scene names. The only OBS-facing promise is: **`scene apply <name>` is a stable cue**.
- Per-camera delay, ordered waits for motion complete, or VISCA ACK handling.
- Manual COM-port identity migration for VISCA.
- Changing `.ptzc` into a versioned `{ "version", "cameras", "scenes" }` envelope. Reserved-key sibling is enough and keeps existing files loading.

## Consequences

**Positive**

- One-word cues without colliding preset names across cameras.
- Scenes travel with the collection (`preset save` / `load` / `browse`).
- Scenes can be listed while a camera is dark; apply degrades per missing member.
- OBS integration, if we do it, is “call `scene apply`,” not a second grouping model.

**Negative / follow-through**

- `PresetStore` load/save must know reserved keys and a `SceneStore`. Save must stop being “dump every connected serial and nothing else.”
- `Context` grows a scene store; commands need it.
- A scene can name a preset that is later `preset discard`ed; apply skips with a warning. We do not cascade-delete bindings.
- VISCA `VISCA:COMx:n` bindings go stale if the COM index changes. Same as aliases/presets for that camera.

## Considered alternatives

| Option | Why not |
|---|---|
| Keep growing snapshots (same preset name everywhere) | Already tried; naming is a bad grouping key. |
| Bake PTZ into the scene | Stable for a locked show; hostile to rehearsal. References match “grouping presets.” |
| Sidecar scene files | Drift from the `.ptzc` they name. |
| Versioned envelope (`cameras` + `scenes`) now | Cleaner, but a needless format break while reserved `"scenes"` works. |
| `scene store` from current PTZ | No “current preset” on `Camera`; would fake it. |
| Hang scenes on `Camera` | A scene is cross-camera by definition; unplug would drop the grouping. |
| Convert the app into an OBS plugin to get OBS scenes | Wrong layer. OBS should *call* `scene apply`, not own PTZ. |

## Implementation sketch (not part of the decision, order of work)

1. `Scene` + `SceneStore`; wire through `Context`.
2. Reserved `"scenes"` key in `PresetStore` load/save (replace semantics, all-or-nothing parse).
3. `scene add` / `remove` / `discard` / `list` / `show` / `help`.
4. `scene apply` (skip missing members).
5. README + `preset help` note that save/load include scenes.
6. Leave `snapshot *` untouched.

## References

- [README.md](../../README.md) — current preset file format and load-replace behavior
- [cameras/PresetStore.h](../../cameras/PresetStore.h) — persistence
- [commands/snapshot/ApplyCommand.cpp](../../commands/snapshot/ApplyCommand.cpp) — today’s same-name cue
- [REFACTORING.md](../../REFACTORING.md) — item 7
