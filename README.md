# PTZ Commander

A C++ command-line tool for controlling PTZ (Pan-Tilt-Zoom) cameras, built with extensibility in mind. Supports OBSBOT Tiny 2/4K cameras via the OBSBOT SDK (`libdev.dll`), with plans for Lumens, VISCA, PELCO-D, and PELCO-P. Run from a REPL—type commands, move cameras, store presets, and save/load presets to named `.ptzc` files (e.g. `presets.ptzc`, `my-show.ptzc`).

## Basic Operation

- **Build**: Requires C++20, CMake, and `libdev.dll` (OBSBOT SDK). Clone, drop `libdev.dll` in `out/build/x64-Release/`, then configure + build with your generator (the repo uses Ninja + Visual Studio via `CMakeSettings.json`):
  - Example: `cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build out/build/x64-Release`
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
| `preset discard`               | ```<id|*> <name>```                         | Removes a named preset from memory.                                        |
| `preset list`                  | None                                    | Lists all presets for all cameras.  |
| `preset load`                  | `<file>` (optional)                     | Loads presets from a `.ptzc` file into memory (defaults to `presets.ptzc`). Merges with existing presets; same-named presets are overwritten. |
| `preset save`                  | `<file>` (optional)                     | Saves current in-memory presets to a `.ptzc` file (defaults to `presets.ptzc`). |
| `preset store`                 | ```<id|*> <name>```                         | Stores current PTZ as a named preset in memory—e.g., ```"intro"```.            |

### Preset Files

Presets are stored in JSON files using the `.ptzc` extension ("PTZ Commander").

- `preset save my-show` → writes `my-show.ptzc`
- `preset load conference` → reads `conference.ptzc` (`.ptzc` is added automatically if omitted)
- When no filename is given, the default is `presets.ptzc`

This allows multiple independent preset collections (different shows, venues, camera configurations, etc.). `preset load` merges into the current in-memory presets; use `preset discard` or restart the app to start fresh.

## Notes
- **Tech Debt**: Preset name dupes—first match wins—alias collisions—first camera wins—<10 cameras—O(n) lookups.
- **Dependencies**: `nlohmann/json`—`$PROJ_DIR/include/nlohmann/json.hpp`—CMake—`include_directories(include)`.
- **Future**: `push/pull/pan`—speed control—Scenes—Pinocchio’d for now!
