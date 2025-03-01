# PTZ Commander

A C++ command-line tool for controlling PTZ (Pan-Tilt-Zoom) cameras, built with extensibility in mind. Supports OBSBOT Tiny 2/4K cameras via the OBSBOT SDK (`libdev.dll`), with plans for Lumens, VISCA, PELCO-D, and PELCO-P. Run from a REPL—type commands, move cameras, store presets, and save/load to `preset.json`.

## Basic Operation

- **Build**: Requires C++17, CMake, and `libdev.dll` (OBSBOT SDK). Clone, drop `libdev.dll` in `out/build/x64-Release/`, and:
  - cmake -S . -B build
  - cmake --build build --config Release
- **Run**: ./out/build/x64-Release/ptz_commander—REPL starts—`> ` prompt—type commands—`exit` to quit.
- **Camera Detection**: Threaded—`ObsbotCameraAdapter`—auto-adds/removes cameras—logs on connect/disconnect.
- **Input**: `<serialNumber>`—SN (e.g., `RMOWTHF7211JGR`), alias (e.g., `"LeftCam"`), or name (e.g., `"OWB-2105-CE"`)—`*` for all cameras.

## Commands

### Camera

The `id` argument can be the camera's alias, serial number, or name. These values can be found with the `camera list` command.

The `pan` argument is a floating-point value between -180 and 180, where 0.0 is the camera's center position.

The `tilt` arguments are floating-point values between -45.0 and 45.0, where 0.0 is the camera's center position. 

The `zoom` argument is an integer between 0 and 100, where 0 is the camera's minimum zoom and 100 is the maximum.

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `camera alias`                 | `<id> <alias> `                      | Sets a friendly alias for a camera — e.g., ```camera alias RMOWTHF7211JGR "LeftCam"```. |
| `camera list`                  | None                                    | Lists all available cameras. |
| `camera move`                  | ```<id|*> <pan> <tilt> <zoom>```            | Moves camera(s) to PTZ—```pan```, ```tilt``` (float), ```zoom``` (int, 0-100). Asterisk moves all cameras.          |
| `camera show`                  | ```<id|*>```                                | Shows camera details.           |


### Preset

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `preset apply`                 | ```<id|*> <name>```                         | Applies a named preset — moves camera(s) to stored PTZ.                      |
| `preset discard`               | ```<id|*> <name>```                         | Removes a named preset from memory.                                        |
| `preset list`                  | None                                    | Lists all presets for all cameras.  |
| `preset load`                  | None                                    | Loads presets into memory — overwrites existing presets.  |
| `preset save`                  | None                                    | Saves all presets — overwrites existing file.               |
| `preset store`                 | ```<id|*> <name>```                         | Stores current PTZ as a named preset in memory—e.g., ```"intro"```.            |

## Notes
- **Tech Debt**: Preset name dupes—first match wins—alias collisions—first camera wins—<10 cameras—O(n) lookups.
- **Dependencies**: `nlohmann/json`—`$PROJ_DIR/include/nlohmann/json.hpp`—CMake—`include_directories(include)`.
- **Future**: `push/pull/pan`—speed control—Scenes—Pinocchio’d for now!
