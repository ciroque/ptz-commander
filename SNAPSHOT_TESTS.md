# Snapshot Command Test Cases

## Test Suite: snapshot apply

### TC1: snapshot apply with valid preset name
**Given:** 
- At least one camera is connected
- Preset "podium" exists on all connected cameras

**When:** User executes `snapshot apply podium`

**Then:**
- Preset "podium" is applied to ALL cameras
- Success message is displayed
- Exit code: 0

**Status:** ❌ NOT IMPLEMENTED

---

### TC2: snapshot apply with missing arguments
**Given:** User is at the command prompt

**When:** User executes `snapshot apply` (no preset name)

**Then:**
- Usage message is displayed: `Usage: snapshot apply <name> [delay_ms]`
- No cameras are affected
- Exit code: 0 (informational, not error)

**Status:** ❌ NOT IMPLEMENTED

---

### TC3: snapshot apply with unknown preset
**Given:** 
- At least one camera is connected
- Preset "nonexistent" does NOT exist on any camera

**When:** User executes `snapshot apply nonexistent`

**Then:**
- Error message indicates preset not found for each camera
- Overall failure message is displayed
- Exit code: 0

**Status:** ❌ NOT IMPLEMENTED

---

### TC4: snapshot apply with optional delay
**Given:**
- Multiple cameras are connected
- Preset "wide" exists on all cameras

**When:** User executes `snapshot apply wide 500`

**Then:**
- Preset "wide" is applied to each camera with 500ms delay between applications
- Success message is displayed

**Status:** ❌ NOT IMPLEMENTED

---

### TC5: snapshot apply with no cameras connected
**Given:** No cameras are connected

**When:** User executes `snapshot apply podium`

**Then:**
- Error message: "No cameras found to apply preset."
- Exit code: 0

**Status:** ❌ NOT IMPLEMENTED

---

## Test Suite: snapshot help

### TC6: snapshot help displays usage
**Given:** User is at the command prompt

**When:** User executes `snapshot help`

**Then:**
- Help text is displayed showing:
  - `snapshot apply <name> [delay_ms]` - description
  - `snapshot help` - description

**Status:** ❌ NOT IMPLEMENTED

---

## Test Suite: Unknown snapshot commands

### TC7: Unknown snapshot subcommand
**Given:** User is at the command prompt

**When:** User executes `snapshot foobar`

**Then:**
- Error message: "Unknown command: snapshot foobar"

**Status:** ✅ EXISTING BEHAVIOR (CommandHandler handles this)

---

## Equivalence Tests

### TC8: snapshot apply behaves like preset apply * 
**Given:** 
- 3 cameras connected
- Preset "closeup" exists on all 3 cameras

**When:** 
1. Note current PTZ positions of all cameras
2. Execute `snapshot apply closeup`
3. Reset cameras to different positions
4. Execute `preset apply * closeup`

**Then:**
- Both commands produce identical results
- Both apply preset to all 3 cameras
- Output messages are consistent in style

**Status:** ❌ NOT IMPLEMENTED

---

## TDD Progress Tracking

- [ ] Phase 1: RED - Tests documented, commands fail ❌
- [ ] Phase 2: GREEN - Minimal implementation, tests pass ✅
- [ ] Phase 3: REFACTOR - Code cleaned, tests still pass ✅
