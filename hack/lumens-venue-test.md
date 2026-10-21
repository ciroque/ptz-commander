# Lumens VC-A51P venue test

Copy `lumens-probe.ps1` and this file to the laptop. The script runs on Windows
PowerShell 5.1 as it comes and needs nothing installed.

## 1. Try PTZ Commander first

```
camera scan visca-ip
camera add scanned <n> lumens-vc-a51p
camera move <id> 0 0 0             # should go to home
camera calibrate <id> 340 30 90    # only when it's not live
setup save
```

- If the scan misses it but you can find its IP, try
  `camera add visca-ip <ip> 52381 lumens-vc-a51p`. The camera's on-screen menu
  should show the IP, or the building's router will.
- Use `340 30 90` for calibrate, not the defaults: the Lumens tilts 30° down and
  90° up. With the defaults, tilt degrees would be off.
- Calibration sweeps the camera through its full range for about 25 s and
  finishes at home with zoom fully wide.
- Calibration isn't saved until `setup save`.

## 2. If that fails, close PTZ Commander and run the script

Close PTZ Commander first: it holds the COM ports while it runs, and the script
gets "access denied" on a port the app has open.

```
powershell -ExecutionPolicy Bypass -File .\lumens-probe.ps1 -CameraIp <ip>
```

- Leave off `-CameraIp` if you don't know the address.
- With a serial cable plugged in, it probes every COM port at 9600 and 38400
  baud by default (the two rates the manual lists). `-ComPort COMn` limits it
  to one port.
- Add `-AllowMove` only while the camera isn't live. It then drives the camera
  for about 20 s, in small steps, and ends at home with zoom wide.

## 3. Bring back the log

The log is `lumens-probe-<time>.txt` on the Desktop.

## What the script records

- **Network:** the laptop's IPs and subnets, the Wi-Fi network name (venue
  Wi-Fi is often a guest network, separate from the camera's), the gateway,
  and the ARP table so the camera's MAC can be picked out. It also sends a
  Sony-style discovery broadcast.
- **The camera's address,** if given: ping, the open TCP ports (web page, RTSP
  and so on), then VISCA-IP inquiries sent both the way the app sends them and
  in raw form, plus 20 inquiries in a row to measure reliability and latency.
- **Serial:** every COM port and its adapter type. On each port it sends the
  address-set and IF_Clear broadcasts, then tries camera addresses 1 to 7
  until one answers, then repeats the 20 inquiries.
- **The movement test (`-AllowMove`):** the moves a serial `camera calibrate`
  would rely on.
  - It logs every reply to Home and to the drive commands, with timings.
  - It polls position while panning and tilting each way, and during a pan
    and zoom at the same time, which is what the calibrator does.
  - It makes an absolute move to raw 0/0, which should be home on the Lumens
    and is what `camera move <id> 0 0 0` sends.

Read-only otherwise: without `-AllowMove` it only sends inquiries, plus the two
serial broadcasts.

## For serial

Check the connector on the back of the Lumens before you go. The manual covers
the RS-232 settings (8N1, 9600 or 38400 baud) but not which connector the
camera has, so you may need an adapter cable to plug the CH340 in.

## Tested so far

- **VISCA-IP path:** read-only, against the Tenveo. 20 of 20 inquiries
  answered, with correct positions.
- **Serial path:** ran on COM1, which has nothing connected, so no replies
  were expected.
- **Not run:** the movement test.
