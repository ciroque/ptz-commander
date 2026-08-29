# PTZ Commander

A C++ command-line tool for controlling PTZ (Pan-Tilt-Zoom) cameras, built with extensibility in mind. Supports OBSBOT Tiny 2/4K cameras via the OBSBOT SDK (`libdev.dll`), with plans for Lumens, VISCA, PELCO-D, and PELCO-P. Run from a REPL—type commands, move cameras, store presets, and save/load presets to named `.ptzc` files (e.g. `presets.ptzc`, `my-show.ptzc`).

## Basic Operation

- **Build**: Requires C++20, CMake, and `libdev.dll` (OBSBOT SDK). Clone, drop `libdev.dll` in `out/build/x64-Release/`, then configure + build with your generator (the repo uses Ninja + Visual Studio via `CMakeSettings.json`):
  - Example: `cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build out/build/x64-Release`
- **Installer (Windows)**: After building the x64-Release configuration, run `installer\build-installer.bat` (from the `installer` directory). This uses `cmake --install` to stage files and produces `PTZCommander.msi` (using WiX). The installer adds the tool to Program Files, appends the install directory to the system PATH, and always creates a Start Menu entry (under "PTZCommander"). A checkbox on the final "Ready to install" page controls whether a desktop shortcut (public desktop) is also created (default: checked). You can still force it from the command line: `msiexec /i PTZCommander.msi DESKTOPSHORTCUT=0`.
- **Run**: ./out/build/x64-Release/ptz_commander—REPL starts—`> ` prompt—type commands—`exit` to quit.
- **Camera Detection**: Threaded—`ObsbotCameraAdapter`—auto-adds/removes cameras—logs on connect/disconnect.
- **Input**: `<serialNumber>`—SN (e.g., `RMOWTHF7211JGR`), alias (e.g., `"LeftCam"`), or name (e.g., `"OWB-2105-CE"`)—`*` for all cameras.

## Commands

### Camera

The `id` argument can be the camera's alias, serial number, or name. These values can be found with the `camera list` command.

The `pan` argument is a floating-point value between -180 and 180, where 0.0 is the camera's center position.

The `tilt` argument is a floating-point value between -45.0 and 45.0, where 0.0 is the camera's center position. 

The `zoom` argument is an integer between 0 and 100, where 0 is the camera's minimum zoom and 100 is the maximum.

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `camera alias`                 | `<id> <alias> `                      | Sets a friendly alias for a camera — e.g., ```camera alias RMOWTHF7211JGR "LeftCam"```. |
| `camera list`                  | None                                    | Lists all available cameras. |
| `camera move`                  | ```<id|*> <pan> <tilt> <zoom>```            | Moves camera(s) to PTZ — ```pan```, ```tilt``` (float), ```zoom``` (int, 0-100). Asterisk moves all cameras.          |
| `camera show`                  | ```<id|*>```                                | Shows camera details.           |


### Preset

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `preset apply`                 | ```<id|*> <name>```                         | Applies a named preset — moves camera(s) to stored PTZ.                      |
| `preset browse`                | None                                    | Lists `.ptzc` files in `%LOCALAPPDATA%\PTZCommander`.                       |
| `preset discard`               | ```<id|*> <name>```                         | Removes a named preset from memory.                                        |
| `preset list`                  | None                                    | Lists all presets for all cameras.  |
| `preset load`                  | `<file>` (optional)                     | Replaces in-memory presets from a `.ptzc` file (per-camera: `{"alias": "...", "presets": { ... }}`). Cameras not in the file are cleared. Bare filenames default to `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |
| `preset save`                  | `<file>` (optional)                     | Saves current presets + aliases as a self-contained `.ptzc` file (per-camera: alias + presets). Bare filenames default to `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |
| `preset store`                 | ```<id|*> <name>```                         | Stores current PTZ as a named preset in memory—e.g., ```"intro"```.            |

### Preset Files

Presets are stored in JSON files using the `.ptzc` extension ("PTZ Commander").

- `preset browse` → lists `*.ptzc` files in the user data directory
- `preset save my-show` → writes `my-show.ptzc` (in the user data directory)
- `preset load conference` → reads `conference.ptzc` (`.ptzc` is added automatically if omitted)
- When no filename is given, the default is `%LOCALAPPDATA%\PTZCommander\presets.ptzc`
- If you provide a path with directories (e.g. `C:\shows\my-show` or `..\my-show`), it will be used as-is. Bare names are placed in the per-user PTZCommander data folder.

This allows multiple independent preset collections (different shows, venues, camera configurations, etc.). `preset load` replaces the in-memory presets (it does not merge with whatever was already stored).

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
- **Tech Debt**: Preset name dupes—first match wins—alias collisions—first camera wins—<10 cameras—O(n) lookups.
- **Dependencies**: `nlohmann/json`—`$PROJ_DIR/include/nlohmann/json.hpp`—CMake—`include_directories(include)`.
- **Future**: `push/pull/pan`—speed control—Scenes—Pinocchio’d for now!

## VISCA Serial Camera Support (Windows)
Automatic discovery is performed at startup for VISCA-over-serial cameras.

- Preferred hardware: Lumens VC-A51P (or compatible VISCA serial PTZ) connected via Keyspan USA-19H USB serial adapter (VID 06CD / PID 0121). Appears as e.g. COM3.
- Default settings: 9600 baud, 8N1, camera address 1.
- The implementation uses **absolute** Pan/Tilt/Zoom positioning commands only (no velocity "drive + stop" commands). This matches the `camera move` / preset model used by the rest of the tool.
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
