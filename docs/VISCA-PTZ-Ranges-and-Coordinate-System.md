# PTZ Ranges and Coordinate System for VISCA Serial Support

This document describes how ptz-commander represents and maps Pan/Tilt/Zoom (PTZ) positions for VISCA serial cameras, with specific details for the target hardware: **Lumens VC-A51P**.

## 1. Lumens VC-A51P Physical Specifications

From the official Lumens VC-A51P datasheet and user manual (2022):

- **Pan Range**: -170° to +170° (340° total)
- **Tilt Range**: -30° to +90° (120° total)
- **Zoom**: 20x optical
- **Pan/Tilt Speed**: up to 300°/sec
- **Preset Positions**: 128 (internal to camera)
- **Raw positions** (RS158 command set v1.3): two's-complement around home `0x0000`, 160 per degree. Pan `0x95C0` (−170°, left) to `0x6A40` (+170°, right); tilt `0xED40` (−30°, down) to `0x3840` (+90°, up); zoom `0x0000..0x4000`.

These ranges are the default mechanical limits. The camera supports software Pan/Tilt Limit settings (configurable via OSD menu, web interface, or VISCA commands) that can restrict movement further (e.g., Pan Left Limit = -170, Tilt Down Limit = -30 by default in the menu).

**Note**: The exact "center" (0,0) is determined by the camera's internal homing / power-on position. The Home command (VISCA `81 01 06 04 FF`) typically returns the camera to a predefined mechanical home within these limits.

## 2. ptz-commander General PTZ Interface

The base `Camera` abstraction and commands (e.g. `camera move`, `preset store/apply`, `camera show`) use this model (see original README.md and `cameras/Preset.h` / `commands/camera/MoveCommand.cpp`):

- **`pan`**: `float`, in **degrees**. 0.0 = center. 
  - Positive values typically move one direction (right from operator's view?).
  - Negative the other.
  - General expectation in the tool: roughly -180 to +180 (but clamped per camera).
- **`tilt`**: `float`, in **degrees**. 0.0 = center (level).
  - Positive = up? (convention to be confirmed per camera).
  - General expectation: roughly -45 to +45 or wider.
- **`zoom`**: `int`, 0 to 100.
  - 0 = full wide (minimum zoom)
  - 100 = full tele (maximum zoom)
- `setPosition(float pan, float tilt, int zoom)`
- `setZoom(int zoom, int speed)` (speed currently ignored for VISCA absolute)
- `getCurrentPtz()` returns `Ptz { float pan, float tilt; int zoom; }`

These values are **logical / user-facing** degrees and percent. They are **not** raw VISCA protocol units.

**Important for users**:
- The actual supported range is limited by the connected camera hardware.
- Directions (sign of positive pan/tilt) and exact center depend on camera orientation (desktop/ceiling), flip settings, and installation.
- You will need to experiment with your specific setup to learn the mapping (e.g., `camera move VISCA:COM3:1 30 10 50` and observe physical movement).

## 3. VISCA Serial Implementation (cameras/visca/)

`camera move`, presets and scenes use **absolute positioning** only:

- VISCA Absolute Pan/Tilt (`81 01 06 02 ...`) and Zoom Direct (`81 01 04 47 ...`).
- Home command supported.
- Velocity "drive" commands (`81 01 06 01 ...`, `81 01 04 07 ...`) are used only by `camera calibrate` to find the mechanical stops (section 6).

### Mapping: `ViscaProfile` (in `ViscaProfile.h`, converted by `ViscaCommands.cpp`)

Each camera has a `ViscaProfile`: one `ViscaAxis` for pan and one for tilt, plus a raw zoom range.

```cpp
struct ViscaAxis {
    float minDeg, maxDeg;               // degree range exposed to the tool (min = left / down)
    uint16_t minRaw, homeRaw, maxRaw;   // raw VISCA positions at the stops and at home
    bool throughHome;                   // map each side of home separately, so 0° is home
    bool signedRaw;                     // positions are two's-complement (Sony: left of home is 0xF6xx)
};
```

Conversion is linear with clamping. With `throughHome`, degrees ≤ 0 map onto `minRaw..homeRaw` and degrees > 0 onto `homeRaw..maxRaw`, so 0° lands on home even when home isn't the midpoint of the raw range. Zoom maps 0..100 onto `zoomMinRaw..zoomMaxRaw`.

Built-in profiles (`camera add visca-ip <host> [port] <profile>`):

| Profile | Pan | Tilt | Zoom raw | Notes |
|---|---|---|---|---|
| `lumens-vc-a51p` | ±170°, home `0x0000`, stops `0x95C0`/`0x6A40` (signed) | −30..+90°, home `0x0000`, stops `0xED40`/`0x3840` (signed) | `0x0000..0x4000` | From the Lumens command set (RS158 v1.3), 160 raw per degree; not yet checked on a camera |
| `tenveo-30x` | ±175°, home `0x4A6C`, stops `0x0000`/`0x8E3E` | ±90° over `0x0000..0x3C8A` | `0x0000..0x05E3` | Measured by hand |

An unknown profile name is an error. It used to fall back to `lumens-vc-a51p`.

The `lumens-vc-a51p` mapping used to be unsigned `0x0000..0xFFFF` with home at `0x8000`, a guess that put the camera's real home at −170° pan and −30° tilt. Presets are saved in degrees, so `setup save` marks each camera entry on this profile with `"lumens_mapping": 2`. When `setup load` finds an entry on this profile without the marker, it converts that entry's presets from the old mapping through their raw positions, so they keep pointing at the same spot. Saving the setup again writes the marker. Older builds ignore the marker, but they read the degrees with the old mapping, so don't save a converted setup from an older build.

In `setPosition()` / `setZoom()`:
- Input values are clamped to the profile's ranges.
- VISCA absolute command is built and sent over the serial transport (fire-and-forget for now).
- `lastPtz_` is updated with the (clamped) commanded value.
- `getCurrentPtz()` performs live reads via VISCA inquiries (see limitations).

Speeds in absolute commands are currently fixed at `0x10` (medium) for pan/tilt.

### VISCA Protocol Units vs. Degrees

- VISCA uses 16-bit signed-ish positions (sent as 4 nibbles: high nibble first).
- The stops are **defined by the camera**. Some cameras use unsigned positions with home partway along (the Tenveo: `0x0000..0x8E3E`, home `0x4A6C`); others use two's-complement positions around home `0x0000` (the Lumens: left of home reads `0x95C0..0xFFFF`; Sony cameras work the same way with their own stops).
- Different VISCA cameras (Sony, Lumens, etc.) have different physical angle ranges even when using the same VISCA position scale.
- A wrong profile mostly shows up in zoom. On a Tenveo, the Lumens zoom range puts everything above about 9% past full tele. Presets still return to the same spot under a wrong profile, because store and apply convert through the same mapping.

## 4. Coordinate System and Experimentation

Because you have been moving the camera:

- The "center" (pan=0, tilt=0) in ptz-commander is whatever the camera reports/accepts as the VISCA center after a Home command or power cycle.
- Positive/negative directions:
  - Test with small values, e.g.:
    - `camera move <visca-id> 20 0 0`  (pan right or left?)
    - `camera move <visca-id> 0 15 0`  (tilt up or down?)
  - Observe from the operator's perspective (behind the camera, or as seen in the video feed).
- The tool does **not** flip or remap for ceiling mount / image flip settings. Those are camera-internal (VISCA or menu).
- Zoom 0 = widest field of view, 100 = tightest.

**Recommendation while learning**:
- Use the camera's IR remote or web UI to drive to known positions (e.g. presets 1-9 on the camera itself).
- Then in ptz-commander, issue moves with trial values and store working ones as tool presets (`preset store <id> wide-shot`).
- With live `getCurrentPtz()` via inquiries, you can drive the camera to a known position (e.g. using its internal presets or remote), call getCurrentPtz() or move to it, and then `preset store` the accurate values.

## 5. Current Limitations & Future Work (as of this document)

- **getCurrentPtz()**: Performs live VISCA position inquiries (Pan/Tilt Pos Inquiry `81 09 06 12 FF` and Zoom Pos Inquiry `81 09 04 47 FF`). Falls back to cached last-commanded value on failure or timeout. Enables capturing positions set via the camera's own presets/remote into tool presets.
- **No response reading for motion commands**: `setPosition` / `setZoom` (and other commands) are sent fire-and-forget. We do not read ACK (90 41 FF), Completion (90 51 FF), or error responses from the camera. Errors are only detected via serial write failure. (Position inquiries do perform synchronous reads and parse the responses.)
- **Range clamping**: Values outside the configured min/max are silently clamped.
- **Finding serial cameras**: nothing is discovered at startup. `camera scan serial` finds cameras on any adapter by sending a position inquiry, and `camera add serial <COMn>` adds one by port. One camera per COM port; daisy chains aren't supported.
- **Ceiling mount / orientation**: Handled by camera settings; the degree mapping in the tool assumes "normal" desktop orientation.

Error handling for motion commands remains the next priority (per your earlier list), followed by range tuning based on real hardware data.

## 6. Calibration (`camera calibrate`)

Instead of writing a profile for every camera model, measure the camera itself:

```
camera add visca-ip 192.168.1.146
camera calibrate VISCA-IP:192.168.1.146:52381 350 90 90
setup save
```

`ViscaCalibrator` (`ViscaCalibration.cpp`) does this:

1. Checks that the camera answers position inquiries.
2. Sends Home and reads `homeRaw` for pan and tilt.
3. Drives pan left, tilt down and zoom wide together, polling position every 150 ms until it holds still (3 reads within 2 raw units). That gives the min stops and `zoomMinRaw`. Pan and tilt must first move away from home; if either hasn't moved after 2 s, the drive was not carried out and calibration fails.
4. Drives pan right, tilt up and zoom tele the same way (all three must move). That gives the max stops and `zoomMaxRaw`.
5. Sends Home and drives zoom wide (Home only moves pan/tilt), then waits until a position inquiry shows pan/tilt at the measured home and zoom at the wide stop. Only then is the zoom drive stopped.

Every command's reply is read. A completion accepts it; "command buffer full" (`90 6y 03`) is retried every 100 ms for up to 2 s; any other VISCA error fails calibration with its name (for example `command not executable`). On any failure the camera is still stopped and sent home the same way; every stop, Home and zoom-wide command is sent even if an earlier one is refused, so a running zoom drive is always replaced. If its return can't be confirmed within 30 s, the error (or, after a good measurement, a warning) says it may have been left mid-move.

The camera can't report angles, so the degree span comes from the spec sheet: `panSpan` (split evenly around home, 1–360), `tiltDown` and `tiltUp` (each 1–180). Defaults are `340 90 90`. Both axes map through the measured home, so 0° is always home. Each axis uses unsigned raw positions if home lies between the stops that way; otherwise it uses signed. Each side of home must have at least 16 raw units of travel; a camera whose home sits on a stop can't be calibrated. The same rules are checked for calibrations read from `.ptzc` files.

Calibrating the Tenveo 30x reproduces the hand-measured `tenveo-30x` stops (pan and zoom exact, tilt max within 6 units) in about 25 seconds, ending at home with zoom fully wide.

Things to know:
- **Presets keep their spot.** They're stored in degrees, so applying a new profile converts each preset through its raw position.
- **Persistence.** The measured profile is saved as a `calibration` object on the camera's `visca_ip` entry in the `.ptzc` file, and is used in place of the named profile on load. `setup load` parses the whole file first, checks every `visca_ip` entry's profile, and only then adds cameras or moves an already-added camera onto the file's profile before applying its presets. A parse error (including a camera entry that is not an object, or a calibration that fails these checks) or an unknown profile leaves everything unchanged, and the error says what was wrong.
- **Soft limits and digital zoom count.** If the camera has pan/tilt limits set, or digital zoom enabled, calibration measures those as the stops.
- **VISCA-IP only, so far.** Serial cameras are now saved in setups, so a calibration could persist for them, but calibrating over serial isn't built yet.

## 7. Related Files

- `cameras/visca/ViscaProfile.h` — `ViscaProfile` / `ViscaAxis` and the built-in profiles
- `cameras/visca/ViscaCalibration.h` + `.cpp` — `camera calibrate`'s measurement routine
- `cameras/visca/ViscaCommands.h` + `.cpp` — mapping + packet builders + reply parsing
- `cameras/visca/ViscaCamera.cpp` — uses the commands for `setPosition`/`setZoom`
- `cameras/Camera.h` + `Preset.h` — base interface
- `commands/camera/MoveCommand.cpp` — parses user `camera move` args
- `README.md` — general usage of pan/tilt/zoom arguments
- `cameras/visca/ViscaSerialScanner.*` — how COM ports are listed and scanned for cameras (unrelated to PTZ values)

## 8. References

- Lumens VC-A51P Datasheet (2022)
- Lumens VC-A51P / VC-A51PN User Manual (English, 2022-0127)
- Sony VISCA Command List (for protocol details on absolute commands and inquiries)

---

**Current implementation status**:
- Live `getCurrentPtz()` via VISCA inquiries (Pan/Tilt Pos + Zoom Pos) is implemented and converts responses back to tool units. Falls back gracefully to cache.
- This should allow you to position the camera using its own presets/remote, then use `getCurrentPtz()` (or move + store) to capture accurate values into ptz-commander presets.

**Recommended next** (per your earlier list):
- Error / response handling (read ACK, Completion, and error packets from commands).
- Range tuning / coordinate system validation based on your testing with the VC-A51P.
- Anything else you discover while playing.

Let me know the next item!
