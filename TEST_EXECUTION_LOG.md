# Manual Test Execution Log - RED Phase

## Expected Behavior (RED)
All snapshot commands should fail with "Unknown command" because they don't exist yet.

## Test Execution Instructions

### Setup
1. Build the project: `cmake --build build`
2. Run the application: `.\build\Debug\ptz_commander.exe` (or Release)
3. If cameras are connected, note their serial numbers

### Execute Tests

#### Test 1: snapshot apply (no args)
```
> snapshot apply
```
**Expected (RED):** `Unknown command: snapshot apply`
**Actual:** [RECORD RESULT HERE]

---

#### Test 2: snapshot apply podium
```
> snapshot apply podium
```
**Expected (RED):** `Unknown command: snapshot apply`
**Actual:** [RECORD RESULT HERE]

---

#### Test 3: snapshot help
```
> snapshot help
```
**Expected (RED):** `Unknown command: snapshot help`
**Actual:** [RECORD RESULT HERE]

---

#### Test 4: Verify preset apply * still works (baseline)
```
> preset apply * podium
```
**Expected (BASELINE):** Works as normal (or shows preset not found if podium doesn't exist)
**Actual:** [RECORD RESULT HERE]

---

## RED Phase Completion Criteria
✅ All snapshot commands fail with "Unknown command"
✅ Existing preset commands still work
✅ Ready to implement minimal GREEN solution

---

## GREEN Phase Checklist (Next)
- [x] Create `commands/snapshot/ApplyCommand.h`
- [x] Create `commands/snapshot/ApplyCommand.cpp`
- [x] Create `commands/snapshot/HelpCommand.h`
- [x] Create `commands/snapshot/HelpCommand.cpp`
- [x] Register commands in `CommandHandler`
- [x] Update CMakeLists.txt
- [x] Build successful ✅
- [ ] Re-run tests - all should pass

---

## GREEN Phase Test Execution

### Setup
1. ✅ Build successful
2. Run: `.\build\Debug\ptz_commander.exe`

### Execute GREEN Tests

#### Test 1: snapshot help
```
> snapshot help
```
**Expected (GREEN):** Display snapshot command help
**Actual:** [VERIFY THIS]

---

#### Test 2: snapshot apply (no args)
```
> snapshot apply
```
**Expected (GREEN):** `Usage: snapshot apply <name> [delay_ms]`
**Actual:** [VERIFY THIS]

---

#### Test 3: snapshot apply with preset name
```
> snapshot apply podium
```
**Expected (GREEN):** Applies preset to all cameras (or shows "Preset not found" if it doesn't exist)
**Actual:** [VERIFY THIS]

---

#### Test 4: Verify equivalence with preset apply *
```
> preset apply * podium
> snapshot apply podium
```
**Expected (GREEN):** Both commands produce similar results
**Actual:** [VERIFY THIS]
