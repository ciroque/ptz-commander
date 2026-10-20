# PTZ Commander

A C++ command-line tool for controlling PTZ (Pan-Tilt-Zoom) cameras, built with extensibility in mind. Supports OBSBOT Tiny 2/4K cameras via the OBSBOT SDK (`libdev.dll`), with plans for Lumens, VISCA, PELCO-D, and PELCO-P. Run from a REPL—type commands, move cameras, store presets, compose scenes, and save/load setups to named `.ptzc` files (e.g. `presets.ptzc`, `my-show.ptzc`).

## Basic Operation

- **Build**: Requires C++20, CMake, and `libdev.dll` (OBSBOT SDK). Clone, drop `libdev.dll` in `out/build/x64-Release/`, then configure + build with your generator (the repo uses Ninja + Visual Studio via `CMakeSettings.json`; the `x64-Release` configuration is **RelWithDebInfo**):
  - Example: `cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo && cmake --build out/build/x64-Release`
- **Installer (Windows)**: After building RelWithDebInfo into `out/build/x64-Release`, run `installer\build-installer.bat` (from the `installer` directory). This uses `cmake --install ... --config RelWithDebInfo` to stage files and produces `PTZCommander.msi` (using WiX). The installer adds the tool to Program Files, appends the install directory to the system PATH, and always creates a Start Menu entry (under "PTZCommander"). A checkbox on the Options page controls whether a desktop shortcut (public desktop) is also created (default: checked). You can still force it from the command line: `msiexec /i PTZCommander.msi DESKTOPSHORTCUT=0`. The MSI also installs `obs-panel\` (OBS Custom Browser Dock HTML plus an operator Getting Started guide) and offers to open that guide when setup finishes (default: checked). Pushing a version tag (`v1.2.3`) runs a GitHub Action that builds the MSI and attaches it to a GitHub Release. The MSI product version is that tag as four numbers (`v0.3.1` becomes `0.3.1.0`). Installers built before this stamp all reported `1.0.0.0`, which Windows treats as newer than `0.x`, so those copies must be uninstalled once before a tag-stamped MSI will replace them. Operators should use the installed Getting Started document, not this README.
- **Run**: ./out/build/x64-Release/ptz_commander—REPL starts and prints the build version—`> ` prompt—type commands—`exit` to quit.
- **Camera Detection**: Threaded—`ObsbotCameraAdapter`—auto-adds/removes cameras—logs on connect/disconnect.
- **Input**: `<serialNumber>`—SN (e.g., `RMOWTHF7211JGR`), alias (e.g., `"LeftCam"`), or name (e.g., `"OWB-2105-CE"`)—`*` for all cameras.

## Commands

### Version

| Command          | Arguments | Description                                                                 |
|------------------|-----------|-----------------------------------------------------------------------------|
| `version`        | None      | Prints the version, such as `0.3.1`. |
| `version check`  | None      | Compares that version with the latest published version. Prints where to get it when a newer one exists. Does not download or install. |

If the check cannot reach the published version, it says so and still prints the local version.

### Camera

The `id` argument can be the camera's alias, serial number, or name. These values can be found with the `camera list` command.

The `pan` argument is a floating-point value between -180 and 180, where 0.0 is the camera's center position.

The `tilt` argument is a floating-point value between -45.0 and 45.0, where 0.0 is the camera's center position. 

The `zoom` argument is an integer between 0 and 100, where 0 is the camera's minimum zoom and 100 is the maximum.

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `camera alias`                 | `<id> <alias>`                          | Sets a unique friendly alias for a camera — e.g., ```camera alias RMOWTHF7211JGR "LeftCam"```. Rejected if the alias is `*` or is already used as another camera's alias, serial, or name. |
| `camera list`                  | None                                    | Lists all available cameras. |
| `camera add visca-ip`          | `<host> [port] [profile]`               | Adds a VISCA-over-IP camera (default port 52381, profile tenveo-30x or lumens-vc-a51p). Survives `setup save` / `setup load`. |
| `camera scan visca-ip`         | `[subnet] [port]`                       | Lists the VISCA-over-IP cameras that answer on the local networks (each narrowed to a /24), or on a subnet such as `192.168.1.0/24` (down to /16). Default port 52381. Only sends position inquiries: nothing is added or moved. Rows are numbered for `camera add scanned`. |
| `camera add scanned`           | `<n...\|*> [profile]`                  | Adds cameras by their row number in the last `camera scan visca-ip` (e.g. `camera add scanned 1 3`, or `*` for every row). Rows already added are skipped. Default profile tenveo-30x. |
| `camera calibrate`             | `<id> [panSpan] [tiltDown] [tiltUp]`    | Measures a VISCA-IP camera's pan, tilt and zoom stops and switches it to the measured profile. **Moves the camera through its full range** (about 25 seconds), then leaves it at home with zoom fully wide. The degree arguments come from the spec sheet (defaults `340 90 90`). Existing presets keep their positions. |
| `camera move`                  | ```<id|*> <pan> <tilt> <zoom>```            | Moves camera(s) to PTZ — ```pan```, ```tilt``` (float), ```zoom``` (int, 0-100). Asterisk moves all cameras.          |
| `camera show`                  | ```<id|*>```                                | Shows camera details.           |


### Preset

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `preset apply`                 | ```<id|*> <name>```                         | Applies a named preset — moves camera(s) to stored PTZ.                      |
| `preset discard`               | ```<id|*> <name>```                         | Removes a named preset from memory.                                        |
| `preset list`                  | None                                    | Lists all presets for all cameras.  |
| `preset store`                 | ```<id|*> <name>```                         | Stores current PTZ as a named preset in memory—e.g., ```"intro"```.            |

### Scene

A scene is a named set of camera → preset bindings. Bindings are resolved when you apply the scene (they are not baked PTZ values). `*` is not accepted. Persist with `setup save` / `setup load` (reserved top-level `"scenes"` key in the `.ptzc` file).

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `scene add`                    | ```<name> <id> <preset> [<id> <preset> ...]``` | Adds or replaces bindings. Camera id is alias, serial, or name. Stored as serial. |
| `scene apply`                  | ```<name>```                            | Moves each bound camera to that camera's named preset. Missing camera/preset is skipped. |
| `scene discard`                | ```<name>```                            | Deletes the scene.                                                          |
| `scene list`                   | None                                    | Lists scene names and binding counts.                                       |
| `scene remove`                 | ```<name> <id>```                       | Removes one camera from a scene.                                            |
| `scene show`                   | ```<name>```                            | Shows each binding (friendly name, serial, preset).                         |
| `scene tour`                   | ```[seconds]```                         | Applies every scene in `scene list` order, waiting between them (not after the last). Default: 7 seconds. REPL is blocked until the tour finishes. |

### Setup

A setup is the on-disk collection: per-camera aliases and presets plus scenes. Files use the `.ptzc` extension ("PTZ Commander").

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `setup list`                   | None                                    | Lists `.ptzc` files in `%LOCALAPPDATA%\PTZCommander`.                       |
| `setup load`                   | `[file]`                                | Replaces in-memory presets and scenes from a `.ptzc` file. Cameras not in the file are cleared of presets; a missing `"scenes"` key clears scenes. Default: `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |
| `setup save`                   | `[file]`                                | Writes current aliases, presets, and scenes to a `.ptzc` file. Default: `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |

- `setup save my-show` → writes `my-show.ptzc` (in the user data directory)
- `setup load conference` → reads `conference.ptzc` (`.ptzc` is added automatically if the path has no extension)
- When no filename is given, the default is `%LOCALAPPDATA%\PTZCommander\presets.ptzc`
- If you provide a path with directories (e.g. `C:\shows\my-show` or `..\my-show`), it will be used as-is. Bare names are placed in the per-user PTZCommander data folder.

`setup load` replaces in-memory presets and scenes (it does not merge).

## WebSocket

On start the REPL listens on `ws://127.0.0.1:7420`. A client receives the current setup JSON on connect (and again whenever setup changes). Clients may apply a scene — the same cue as `scene apply <name>`:

```json
{ "op": "scene.apply", "name": "sermon" }
```

Plain text `scene apply sermon` is also accepted. Scene names cannot contain spaces. The reply is `{ "ok": true, "op": "scene.apply", "name": "sermon" }` or `{ "ok": false, "error": "..." }`. Apply still prints on the REPL. Other commands are not accepted over the socket.

The OBS dock is `obs-panel/index.html` (shipped in the MSI). `web/index.html` is a local test page, not the panel.

Example `.ptzc` file structure (per-camera):
```json
{
  "RMOWTHF7211JGR": {
    "alias": "LeftCam",
    "presets": {
      "intro": { "name": "intro", "pan": 0.0, "tilt": 0.0, "zoom": 0 },
      "close": { "name": "close", "pan": 12.5, "tilt": -3.2, "zoom": 65 }
    }
  }
}
```

## Notes
- **Tech Debt**: Preset name dupes—first match wins—<10 cameras—O(n) lookups.
- **Dependencies**: `nlohmann/json`—`$PROJ_DIR/include/nlohmann/json.hpp`—CMake—`include_directories(include)`.
- **Future**: `push/pull/pan`—speed control.

## VISCA Serial Camera Support (Windows)
Automatic discovery is performed at startup for VISCA-over-serial cameras.

- Preferred hardware: Lumens VC-A51P (or compatible VISCA serial PTZ) connected via Keyspan USA-19H USB serial adapter (VID 06CD / PID 0121). Appears as e.g. COM3.
- Default settings: 9600 baud, 8N1, camera address 1.
- Moves use **absolute** Pan/Tilt/Zoom positioning commands. This matches the `camera move` / preset model used by the rest of the tool. (Velocity "drive + stop" commands are only used by `camera calibrate`.)
- Discovered cameras get synthetic serial numbers of the form `VISCA:COM3:1` (usable for `camera move`, `preset store/apply`, aliases, etc.).
- Multiple VISCA cameras and/or OBSBOT cameras can coexist.

Example (typical auto-discovered usage):
```
camera list
camera move VISCA:COM3:1 12.5 -3.0 40
preset store VISCA:COM3:1 intro
preset apply VISCA:COM3:1 intro
```

Manual port configuration remains possible in future via CLI/config (the discovery and camera code are designed around defaults + `ViscaConfig` so they can be extended without major surgery).

## VISCA-over-IP Camera Support

Find the camera, add it by its row in the scan, then let `camera calibrate` measure its ranges instead of relying on a built-in profile:

```
camera scan visca-ip
camera add scanned 1
camera calibrate VISCA-IP:192.168.1.146:52381 350 90 90
setup save
```

Calibration moves the camera to every mechanical stop and through its full zoom (about 25 seconds), then leaves it at home with zoom fully wide. The numbers are the spec-sheet pan span and tilt down/up angles; the camera can only report raw positions, not degrees. The measured profile is saved with the setup. See `docs/VISCA-PTZ-Ranges-and-Coordinate-System.md` for details.

`camera scan visca-ip` sends a VISCA-over-IP position inquiry to every address in the scanned networks and lists each device that replies, with the position it reported. A /24 takes under 2 seconds, most of it the 1.5 s spent listening for replies. Any VISCA reply counts, including an error, because some cameras (the Tenveo among them) reject the usual version inquiry. Cameras on another subnet can be found by naming it, e.g. `camera scan visca-ip 10.1.20.0/24`. Sony's discovery broadcast isn't used: the Tenveo doesn't answer it, and broadcasts don't cross routers.

The row numbers belong to the last scan in the current session: a new scan replaces them, and they aren't saved. `camera add visca-ip <host>` still works for a camera you already know the address of. A scan can't tell models apart, so cameras added from it get the tenveo-30x profile unless one is given; calibrate any other camera.
