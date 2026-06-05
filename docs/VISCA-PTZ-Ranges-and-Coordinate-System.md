# PTZ Ranges and Coordinate System for VISCA Serial Support

This document describes how ptz-commander represents and maps Pan/Tilt/Zoom (PTZ) positions for VISCA serial cameras, with specific details for the target hardware: **Lumens VC-A51P**.

## 1. Lumens VC-A51P Physical Specifications

From the official Lumens VC-A51P datasheet and user manual (2022):

- **Pan Range**: -170° to +170° (340° total)
- **Tilt Range**: -30° to +90° (120° total)
- **Zoom**: 20x optical
- **Pan/Tilt Speed**: up to 300°/sec
- **Preset Positions**: 128 (internal to camera)

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

The `ViscaCamera` + `ViscaCommands` implement **only absolute positioning** (as requested):

- Uses VISCA Absolute Pan/Tilt (`81 01 06 02 ...`) and Zoom Direct (`81 01 04 47 ...`).
- Home command supported.
- No velocity "drive" commands (pan left/right etc.) — only absolute targets. This matches the `camera move` + preset model of the rest of the tool.

### Current Mapping (in `ViscaCommands.h` / `.cpp`)

```cpp
namespace defaults {
    constexpr float kPanMinDeg   = -170.0f;
    constexpr float kPanMaxDeg   =  170.0f;
    constexpr float kTiltMinDeg  =  -30.0f;
    constexpr float kTiltMaxDeg  =   90.0f;
    constexpr uint16_t kZoomMin  = 0x0000;
    constexpr uint16_t kZoomMax  = 0x4000;  // typical for ~20x optical
}
```

Conversion (linear, with clamping):

- `panDegreesToVisca(float degrees)`: maps `kPanMinDeg .. kPanMaxDeg` → `0x0000 .. 0xFFFF`
  - 0° (center) → approximately `0x8000`
- `tiltDegreesToVisca(float degrees)`: maps `kTiltMinDeg .. kTiltMaxDeg` → `0x0000 .. 0xFFFF`
- `zoomPercentToVisca(int percent)`: maps 0..100 → `0x0000 .. 0x4000`

These constants were chosen to match the **Lumens VC-A51P physical limits** exactly.

In `setPosition()` / `setZoom()`:
- Input values are clamped to the above ranges.
- VISCA absolute command is built and sent over the serial transport (fire-and-forget for now).
- `lastPtz_` is updated with the (clamped) commanded value.
- `getCurrentPtz()` returns the cached `lastPtz_` (optimistic — see limitations below).

Speeds in absolute commands are currently fixed at `0x10` (medium) for pan/tilt.

### VISCA Protocol Units vs. Degrees

- VISCA uses 16-bit signed-ish positions (sent as 4 nibbles: high nibble first).
- 0x0000 and 0xFFFF are the extremes **as defined by the camera** (not necessarily exactly -170° / +170° in all VISCA cameras).
- Different VISCA cameras (Sony, Lumens, etc.) have different physical angle ranges even when using the same VISCA position scale.
- The mapping above is a best-effort linear conversion for the VC-A51P.

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
- Once `getCurrentPtz()` can read live values (see below), this will become much easier.

## 5. Current Limitations & Future Work (as of this document)

- **getCurrentPtz()**: Currently returns the *last successfully sent* PTZ value. Not a live read from the camera. This is why using the camera's own presets to bootstrap tool presets is important right now.
- **No response reading**: Commands are sent but we do not (yet) read ACK (90 41 FF) / Completion (90 51 FF) or error responses. Errors are only detected via serial write failure.
- **No live position inquiry**: We have not implemented VISCA inquiries (`81 09 06 12 FF` for pan/tilt position, `81 09 04 47 FF` for zoom) + parsing of the 9-byte replies.
- **Range clamping**: Values outside the configured min/max are silently clamped.
- **Multiple cameras / non-Keyspan**: Discovery prefers Keyspan USA-19H (VID 0x06CD / PID 0x0121 or name match). Other serial VISCA setups may require manual construction or future CLI support.
- **Ceiling mount / orientation**: Handled by camera settings; the degree mapping in the tool assumes "normal" desktop orientation.

These match the priorities you listed (getCurrentPtz() critical, error handling next, ranges after the doc).

## 6. Tuning the Mapping

The constants live in:

```cpp
// cameras/visca/ViscaCommands.h
namespace defaults { ... }
```

And the conversion functions in `ViscaCommands.cpp`.

You can edit them for:
- Other VISCA camera models with different physical ranges.
- Fine-tuning after real-world testing on the VC-A51P (e.g. if mechanical stops are slightly different, or you want a different "percent" feel for zoom).
- Making them configurable per-camera in the future (e.g. via a `ViscaProfile` struct passed to `ViscaCamera`).

After changing, rebuild and re-test.

## 7. Related Files

- `cameras/visca/ViscaCommands.h` + `.cpp` — mapping + packet builders
- `cameras/visca/ViscaCamera.cpp` — uses the commands for `setPosition`/`setZoom`
- `cameras/Camera.h` + `Preset.h` — base interface
- `commands/camera/MoveCommand.cpp` — parses user `camera move` args
- `README.md` — general usage of pan/tilt/zoom arguments
- `cameras/visca/ViscaDiscovery.*` — how ports are found (unrelated to PTZ values)

## 8. References

- Lumens VC-A51P Datasheet (2022)
- Lumens VC-A51P / VC-A51PN User Manual (English, 2022-0127)
- Sony VISCA Command List (for protocol details on absolute commands and inquiries)

---

**Next steps after this document** (per your list):
- Commit the current changes (including this doc).
- Then tackle `getCurrentPtz()` live reading (inquiries).
- Then error / response handling.
- Ranges tuning based on your experimentation.
- Anything else (manual port override, etc.).

Let me know when you're ready to commit or if you'd like any edits to this document first!