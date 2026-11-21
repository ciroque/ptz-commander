# PTZ Commander

[![CI](https://github.com/ciroque/ptz-commander/actions/workflows/ci.yml/badge.svg)](https://github.com/ciroque/ptz-commander/actions/workflows/ci.yml)

PTZ Commander is a Windows command-line tool that drives PTZ cameras from one REPL. The same commands move an OBSBOT camera over USB, a VISCA camera on a COM port or over IP, or an ONVIF camera. Presets, scenes, and `.ptzc` setup files are shared across all of them.

How it is built (the layers, the VISCA degree mapping and calibration, discovery, ONVIF security, and the test and release pipeline) is in [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Install and start

The MSI installs the tool under Program Files, appends that directory to the system PATH, and adds a Start Menu entry. A checkbox on the Options page also creates a public desktop shortcut (default: checked). `msiexec /i PTZCommander.msi DESKTOPSHORTCUT=0` turns the shortcut off. The installer includes `obs-panel\` (the OBS browser dock and a Getting Started guide). The last page of setup has two checkboxes, both checked by default: show the Getting Started guide, and run PTZ Commander. `msiexec /i PTZCommander.msi RUNPTZCOMMANDER=0` leaves the program unstarted. A quiet install does not show the page, so it does not start the program either.

Installers built before the version stamp all reported `1.0.0.0`, which Windows treats as newer than `0.x`. Uninstall one of those copies once before a newer MSI will replace it.

`ptz_commander.exe` starts the REPL, prints the build version, and waits at `> `. `exit` quits.

OBSBOT cameras are added and removed as they are plugged in. VISCA and ONVIF cameras are not discovered at startup. Use `camera scan` and `camera add`.

A camera id is a serial number, an alias, or a product name. `*` means every camera the command can reach.

## Commands

### Version

| Command          | Arguments | Description                                                                 |
|------------------|-----------|-----------------------------------------------------------------------------|
| `version`        | None      | Prints the version, such as `0.3.1`. |
| `version check`  | None      | Compares that version with the latest published version. Prints where to get it when a newer one exists. Does not download or install. |

If the check cannot reach the published version, it says so and still prints the local version.

### Camera

The `id` argument can be the camera's alias, serial number, or name. These values can be found with `camera list`.

`pan` and `tilt` are degrees. VISCA cameras use the profile they were added with, or the ranges measured by `camera calibrate`. ONVIF pan is continuous from -180 to 180, and tilt is -15 to 90. There is no single tilt interval for every camera.

`zoom` is an integer from 0 to 100 on every camera. 0 is wide and 100 is telephoto.

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `camera alias`                 | `<id> <alias>`                          | Sets a unique friendly alias for a camera — e.g., ```camera alias RMOWTHF7211JGR "LeftCam"```. Rejected if the alias is `*` or is already used as another camera's alias, serial, or name. |
| `camera list`                  | None                                    | Lists all available cameras. A VISCA camera shows as connected only if it answers a position inquiry right then; each one that doesn't adds up to 0.6 s. |
| `camera add visca-ip`          | `<host> [port] [profile]`               | Adds a VISCA-over-IP camera if it answers a position inquiry. Default port 52381. The profile is `tenveo-30x` (the default) or `lumens-vc-a51p`. Survives `setup save` / `setup load`. |
| `camera add serial`            | `<COMn> [baud] [address] [profile]`     | Adds a VISCA camera on a COM port if it answers a position inquiry. Defaults are 9600 baud, address 1, and profile `lumens-vc-a51p`. One camera per port. Survives `setup save` / `setup load`. |
| `camera scan visca-ip`         | `[subnet] [port]`                       | Lists the VISCA-over-IP cameras that answer on the local networks (each narrowed to a /24), or on a subnet such as `192.168.1.0/24` (down to /16). Default port 52381. Only sends position inquiries: nothing is added or moved. Rows are numbered for `camera add scanned`. |
| `camera scan serial`           | `[COMn...]`                             | Lists the COM ports (all, or the ones named) where a VISCA camera answers, trying 9600 and 38400 baud and addresses 1-7. Only sends position inquiries: nothing is added or moved. Rows are numbered for `camera add scanned`. |
| `camera scan onvif`            | None                                    | Lists ONVIF cameras that answer discovery. Nothing is added or moved. Rows are numbered for `camera add scanned`. A fixed camera that answers discovery is listed too; adding it fails when it has no PTZ profile. |
| `camera add onvif`             | `<host> <user> <password> [port]`       | Adds an ONVIF camera (default port 80) if it answers and has a PTZ profile. The password is one word. Survives `setup save` / `setup load`. |
| `camera login`                 | `<id> <password>`                       | Sets the password on an ONVIF camera already in the setup, connects, and makes it active. The password is one word. Use this for a camera `setup load` left inactive. |
| `camera scan *`                | None                                    | Runs `camera scan visca-ip`, `camera scan serial`, and `camera scan onvif` at the same time with their defaults, and lists what all three found in one numbered table. If one scan can't run or fails, it says why under the table and the others' results still show. |
| `camera add scanned`           | `<n...\|*> [profile]`                  | Adds cameras by their row number in the last `camera scan` of any kind (e.g. `camera add scanned 1 3`, or `*` for every row). Rows already added are skipped. A VISCA-IP row defaults to profile `tenveo-30x`, a serial row to `lumens-vc-a51p`. An ONVIF row takes a user and password instead of a profile: `camera add scanned <n> <user> <password>`. `camera add scanned *` adds the VISCA rows and leaves the ONVIF rows for that form. |
| `camera calibrate`             | `<id> [panSpan] [tiltDown] [tiltUp]`    | Measures a VISCA camera's (IP or serial) pan, tilt and zoom stops and switches it to the measured profile. **Moves the camera through its full range** (about 25 seconds), then leaves it at home with zoom fully wide. The degree arguments come from the spec sheet (defaults `340 90 90`). Existing presets keep their positions. |
| `camera move`                  | ```<id\|*> <pan> <tilt> <zoom>```            | Moves camera(s) to an absolute pan, tilt, and zoom. Asterisk moves all cameras. An inactive ONVIF camera is refused until `camera login`. |
| `camera push`                  | ```<id\|*> <zoom> [speed]```                 | Zooms gradually to a target zoom (speed 0–100, default 50). Does nothing if the camera is already there or closer. There is no separate pull command. |
| `camera show`                  | ```<id\|*>```                                | Shows camera details.           |


### Preset

A preset is a named pan, tilt, and zoom on one camera. `preset store` saves where that camera is now, and `preset apply` moves it back. The position is kept in the app and sent as an absolute move. Presets stored on the camera are not imported. Persist with `setup save` / `setup load`.

| Command                        | Arguments                               | Description                                                                 |
|--------------------------------|-----------------------------------------|-----------------------------------------------------------------------------|
| `preset apply`                 | ```<id\|*> <name\|*> [delay_ms]```           | Applies a named preset, or every preset when the name is `*`. `delay_ms` is the pause between presets in that cycle. Preset lookup walks the presets stored on that camera. |
| `preset discard`               | ```<id\|*> <name>```                         | Removes a named preset from memory.                                        |
| `preset list`                  | None                                    | Lists all presets for all cameras.  |
| `preset store`                 | ```<id\|*> <name>```                         | Stores current PTZ as a named preset in memory—e.g., ```"intro"```.            |

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
| `setup load`                   | `[file]`                                | Replaces in-memory presets and scenes from a `.ptzc` file. Cameras not in the file are cleared of presets; a missing `"scenes"` key clears scenes. Asks for an ONVIF password this account cannot use. Default: `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |
| `setup save`                   | `[file]`                                | Writes current aliases, presets, and scenes to a `.ptzc` file. ONVIF passwords are DPAPI blobs for this Windows user. Default: `%LOCALAPPDATA%\PTZCommander\presets.ptzc`. |
| `setup export`                 | `<file>`                                | Writes the same file as `setup save` with the ONVIF passwords left out, so it can move to another PC or Windows account. |

- `setup save my-show` → writes `my-show.ptzc` (in the user data directory)
- `setup export my-show` → writes `my-show.ptzc` with the same cameras, presets, and scenes, and no ONVIF password
- `setup load conference` → reads `conference.ptzc` (`.ptzc` is added automatically if the path has no extension)
- When no filename is given, the default is `%LOCALAPPDATA%\PTZCommander\presets.ptzc`
- If you provide a path with directories (e.g. `C:\shows\my-show` or `..\my-show`), it will be used as-is. Bare names are placed in the per-user PTZCommander data folder.

`setup load` replaces in-memory presets and scenes (it does not merge).

### ONVIF passwords

`setup save` writes each ONVIF password with Windows DPAPI (`CryptProtectData`). The file stores `dpapi:` plus the base64 of that blob, not the password. The blob decrypts only for the Windows user who saved it, which matches `.ptzc` files living in `%LOCALAPPDATA%\PTZCommander`. `setup export <file>` writes the same setup with every ONVIF password left out, so the file can move to another PC or Windows account without carrying a secret.

`setup load` of a missing password, or of a `dpapi:` blob this account cannot decrypt, asks `Password for <user>@<host> (ONVIF, blank to skip):` and does not echo what you type. A password the camera rejects is asked again, up to three times. A blank line, or a load whose input is not a console, leaves that camera inactive: `camera list` shows it, it cannot be moved, and its presets and scene bindings stay. `camera login <id> <password>` sets the password, connects, and makes it active. The next `setup save` stores a password entered at the prompt or at login as a DPAPI blob for the current user. An older plaintext password still loads, and so does a `dpapi:` blob saved by this user. A camera whose stored password decrypts, but that does not answer, is still left out. The password is not logged. The WebSocket snapshot does not include it, and the socket cannot run `setup load`.

## Finding cameras

`camera scan` looks for cameras without adding or moving them. `camera scan *` runs the network, COM port, and ONVIF scans together (about 3 seconds), so you don't need to know which kind of camera is there:

```
camera scan *
camera add scanned *
setup save
```

`camera add scanned *` gives VISCA-IP rows the `tenveo-30x` profile and serial rows `lumens-vc-a51p`. Name a profile to use the other one, or calibrate the camera afterwards. ONVIF rows are listed and then left out of that add, because they need a user and password: `camera add scanned <n> <user> <password>`.

## Serial cameras

Serial cameras aren't looked for at startup. Find the camera, add it by its row in the scan, then save the setup so `setup load` brings it back:

```
camera scan serial
camera add scanned 1
setup save
```

`camera scan serial` sends a position inquiry on every COM port, or only the ones named (`camera scan serial COM3`). It tries 9600 and 38400 baud and camera addresses 1-7, and lists the ports where a camera answered. Any USB serial adapter works. Ports are scanned at the same time, and a silent one takes about 2.5 seconds. Below the table it lists the ports that stayed silent, are in use by another program, weren't found, or already have a camera. Nothing moves.

`camera add serial <COMn> [baud] [address] [profile]` adds a camera whose port you already know, e.g. `camera add serial COM3 9600 1 lumens-vc-a51p`. Defaults are 9600 baud, address 1, and the `lumens-vc-a51p` profile. The camera is added only if it answers, which takes about 2 seconds to give up on a silent one.

- The camera connects through any USB serial adapter. The adapter appears as a COM port, e.g. COM3. The line settings are 8N1.
- **One camera per COM port.** Each camera opens its port exclusively, so cameras daisy-chained on one port aren't supported.
- Serial cameras have ids of the form `VISCA:COM3:1` (port and camera address), usable for `camera move`, `preset store` / `preset apply`, aliases, and the other camera commands. If Windows gives the adapter a different COM number, for example when it's plugged into another USB port, it's a different id: add the camera again, or change the key in the `.ptzc` file.
- `setup save` stores the baud rate and profile in the camera's entry in the `.ptzc` file (`"visca_serial"`). `setup load` adds the camera even if its adapter is unplugged at the time, as it does for a VISCA-IP camera that's switched off. Files saved by earlier versions, which searched for one USB serial adapter at startup, load their `VISCA:COMn:a` cameras at 9600 baud on the `lumens-vc-a51p` profile.
- `camera calibrate VISCA:COM3:1 340 30 90` measures a serial camera's stops the same way as a VISCA-IP camera's (see below), and `setup save` keeps the result in its `"visca_serial"` block. The numbers are the spec-sheet pan span and the tilt down and tilt up angles.
- Moves use absolute pan, tilt, and zoom. Velocity commands are only used by `camera calibrate`.
- Serial cameras, VISCA-IP cameras, ONVIF cameras, and OBSBOT cameras can be used together.

Once added:

```
camera move VISCA:COM3:1 12.5 -3.0 40
preset store VISCA:COM3:1 intro
preset apply VISCA:COM3:1 intro
```

## VISCA over IP

Find the camera, add it by its row in the scan, then let `camera calibrate` measure its ranges instead of relying on a built-in profile:

```
camera scan visca-ip
camera add scanned 1
camera calibrate VISCA-IP:192.168.1.146:52381 350 90 90
setup save
```

Calibration moves the camera to every mechanical stop and through its full zoom (about 25 seconds), then leaves it at home with zoom fully wide. The numbers are the spec-sheet pan span and the tilt down and tilt up angles. The camera reports raw positions, not degrees. The measured profile is saved with the setup.

`camera scan visca-ip` sends a position inquiry to every address in the scanned networks and lists each device that replies, with the position it reported. A /24 takes under 2 seconds, most of it the 1.5 s spent listening for replies. Any VISCA reply counts, including an error, because some cameras reject the usual version inquiry. Cameras on another subnet can be found by naming it, e.g. `camera scan visca-ip 10.1.20.0/24`. A discovery broadcast is not used: some cameras do not answer it, and broadcasts do not cross routers.

The row numbers belong to the last scan in the current session: a new scan replaces them, and they aren't saved. `camera add visca-ip <host>` still works for a camera you already know the address of. A scan can't tell models apart, so a VISCA-IP camera added from it gets the `tenveo-30x` profile unless one is given. Calibrate a camera that does not match that profile.

Both forms of `camera add` send the same position inquiry first and add the camera only if a VISCA reply comes back (an error reply counts), so a wrong address or port fails when you add it instead of leaving a camera that never moves. Waiting for a camera that isn't there takes about 2 seconds. `setup load` doesn't check: a saved camera still loads while it's switched off.

## ONVIF

Find the camera, then add that row with its ONVIF user and password. The password is one word, with no spaces:

```
camera scan onvif
camera add scanned 1 user password
setup save
```

`camera add onvif <host> <user> <password> [port]` adds a camera whose address you already know. The default port is 80. The camera is added only if it answers and exposes a PTZ profile, so a wrong address or a fixed camera fails here instead of showing up as a camera that never moves. `setup load` tries again. A camera with a saved password that is switched off is left out, and the rest of the setup still loads. A camera with no password this account can use stays in the setup as inactive until `camera login`.

Pan is continuous, in degrees from -180 to 180. -180 and 180 are the same heading, and the head is not driven to a stop to find one. Tilt is -15 to 90 (the camera auto-flips past 90). Zoom is 0 to 100, wide to tele. Presets stay in the app and are replayed as an absolute move. Presets stored on the camera are not imported.

`setup save` writes the host, port, user, service path, and password in an `onvif` list in the local `.ptzc` file. The password is a DPAPI blob, described under [ONVIF passwords](#onvif-passwords). It is not logged and is not part of the WebSocket snapshot. Ids look like `ONVIF:192.168.1.211:80`. `camera scan *` includes this discovery along with the VISCA scans.

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

## Tested on these cameras

- Tenveo 30x over VISCA-IP. Position inquiries and absolute moves were checked on that camera, and the `tenveo-30x` profile comes from those measurements.
- OBSBOT cameras over USB. They are added and removed as they are plugged in.
- The `lumens-vc-a51p` profile is taken from the Lumens VC-A51P manual and has been confirmed on that camera.

## Build

The build needs C++20, CMake 3.28, and a Visual Studio x64 toolchain. `libdev.dll` and `libdev.lib` are already in `lib/obsbot/`.

```text
cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build out/build/x64-Release
ctest --test-dir out/build/x64-Release --output-on-failure
```

The executable is `out\build\x64-Release\ptz_commander.exe`.

After a RelWithDebInfo build into `out/build/x64-Release`, run `installer\build-installer.bat` from the `installer` directory. That produces `PTZCommander.msi`. Pushing a version tag such as `v1.2.3` builds the MSI and attaches it to a GitHub Release. The MSI product version is that tag as four numbers (`v0.3.1` becomes `0.3.1.0`).
