<#
Lumens VC-A51P probe for PTZ Commander. Windows PowerShell 5.1, no installs.

Everything is logged to lumens-probe-<time>.txt on the Desktop; bring that file back.

Read-only unless -AllowMove: only inquiries plus the VISCA address-set and
IF_Clear broadcasts on serial. With -AllowMove it also drives the camera for
about 20 s (small pan/tilt/zoom drives, then Home + zoom wide). Don't use it
while the camera is live.

Examples:
  .\lumens-probe.ps1                                # network info, discovery, every COM port
  .\lumens-probe.ps1 -CameraIp 10.0.0.50            # also probe that address over VISCA-IP
  .\lumens-probe.ps1 -ComPort COM4                  # only that COM port
  .\lumens-probe.ps1 -CameraIp 10.0.0.50 -AllowMove # also the movement test

If Windows blocks it: powershell -ExecutionPolicy Bypass -File .\lumens-probe.ps1 ...
#>
param(
    [string]$CameraIp,
    [string]$ComPort,
    [switch]$AllowMove,
    [string]$OutDir = [Environment]::GetFolderPath('Desktop')
)

$ErrorActionPreference = 'Continue'
$log = Join-Path $OutDir ("lumens-probe-{0}.txt" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
Start-Transcript -Path $log | Out-Null
$clock = [Diagnostics.Stopwatch]::StartNew()

function Hex([byte[]]$b) { if (-not $b -or $b.Length -eq 0) { '(none)' } else { ($b | ForEach-Object { $_.ToString('X2') }) -join ' ' } }
function Section([string]$t) { Write-Host ''; Write-Host ('=' * 70); Write-Host "== $t"; Write-Host ('=' * 70) }
function Bytes([string]$s) { [byte[]]($s -split ' ' | ForEach-Object { [Convert]::ToByte($_, 16) }) }

# Split a byte stream into VISCA packets (each ends with FF).
function Packets([byte[]]$b) {
    $out = @(); $cur = New-Object System.Collections.Generic.List[byte]
    foreach ($x in $b) { $cur.Add($x); if ($x -eq 0xFF) { $out += ,($cur.ToArray()); $cur.Clear() } }
    if ($cur.Count -gt 0) { $out += ,($cur.ToArray()) }
    return ,$out
}

# ---------------------------------------------------------------------------
# Transports. Each Xfer sends one VISCA message and returns what came back
# within $ms: @{ Bytes; FirstMs } (FirstMs = -1 if nothing).
# ---------------------------------------------------------------------------
$script:seq = 1
function New-IpLink([string]$ip) {
    $udp = New-Object Net.Sockets.UdpClient
    $udp.Client.Bind((New-Object Net.IPEndPoint ([Net.IPAddress]::Any), 0))
    # Don't let an ICMP port-unreachable kill the socket (SIO_UDP_CONNRESET).
    try { [void]$udp.Client.IOControl(-1744830452, [byte[]](0, 0, 0, 0), $null) } catch {}
    @{ Kind = 'ip'; Udp = $udp; Ep = (New-Object Net.IPEndPoint ([Net.IPAddress]::Parse($ip)), 52381); Name = "VISCA-IP ${ip}:52381" }
}
function New-SerialLink([string]$port, [int]$baud) {
    $sp = New-Object IO.Ports.SerialPort $port, $baud, ([IO.Ports.Parity]::None), 8, ([IO.Ports.StopBits]::One)
    $sp.Handshake = [IO.Ports.Handshake]::None
    $sp.DtrEnable = $true   # as PTZ Commander does
    $sp.RtsEnable = $true
    $sp.ReadTimeout = 50
    $sp.Open()
    @{ Kind = 'serial'; Port = $sp; Name = "$port @$baud" }
}
function Close-Link($link) {
    if ($link.Kind -eq 'ip') { $link.Udp.Close() } else { $link.Port.Close() }
}

# $raw: send over UDP without the 8-byte VISCA-over-IP header.
function Xfer($link, [byte[]]$msg, [int]$ms = 400, [switch]$raw) {
    $got = New-Object System.Collections.Generic.List[byte]
    $sw = [Diagnostics.Stopwatch]::StartNew(); $first = -1
    if ($link.Kind -eq 'ip') {
        while ($link.Udp.Available -gt 0) { $f = New-Object Net.IPEndPoint ([Net.IPAddress]::Any), 0; [void]$link.Udp.Receive([ref]$f) }
        if ($raw) { $pkt = $msg } else {
            $type = if ($msg[1] -eq 0x09) { 0x10 } else { 0x00 }   # 0100 command, 0110 inquiry
            $s = $script:seq; $script:seq++
            $pkt = [byte[]](0x01, $type, 0x00, $msg.Length, (($s -shr 24) -band 0xFF), (($s -shr 16) -band 0xFF), (($s -shr 8) -band 0xFF), ($s -band 0xFF)) + $msg
        }
        $sw.Restart()
        try { [void]$link.Udp.Send($pkt, $pkt.Length, $link.Ep) } catch { Write-Host "   send failed: $($_.Exception.Message)" }
        while ($sw.ElapsedMilliseconds -lt $ms) {
            if ($link.Udp.Available -gt 0) {
                $f = New-Object Net.IPEndPoint ([Net.IPAddress]::Any), 0
                try { $d = $link.Udp.Receive([ref]$f) } catch { Write-Host "   receive error: $($_.Exception.Message)"; break }
                if ($first -lt 0) { $first = $sw.ElapsedMilliseconds }
                if (-not $script:quiet) { Write-Host ("   datagram from {0}: {1}" -f $f, (Hex $d)) }
                if ($d.Length -gt 8 -and $d[0] -eq 0x01) { $got.AddRange([byte[]]$d[8..($d.Length - 1)]) } else { $got.AddRange([byte[]]$d) }
            } else { Start-Sleep -Milliseconds 2 }
        }
    } else {
        $sp = $link.Port
        $sp.DiscardInBuffer()
        $sw.Restart()
        $sp.Write($msg, 0, $msg.Length)
        $buf = New-Object byte[] 256
        while ($sw.ElapsedMilliseconds -lt $ms) {
            if ($sp.BytesToRead -gt 0) {
                $n = $sp.Read($buf, 0, [Math]::Min($buf.Length, $sp.BytesToRead))
                if ($first -lt 0) { $first = $sw.ElapsedMilliseconds }
                for ($i = 0; $i -lt $n; $i++) { $got.Add($buf[$i]) }
            } else { Start-Sleep -Milliseconds 2 }
        }
    }
    @{ Bytes = $got.ToArray(); FirstMs = $first }
}

function Show($label, $r) {
    $t = if ($r.FirstMs -ge 0) { "$($r.FirstMs) ms" } else { '-' }
    Write-Host ("   {0,-28} first byte {1,7}  RX {2}" -f $label, $t, (Hex $r.Bytes))
}

# A VISCA reply from camera address a starts with (a+8)<<4 (90, A0, ...), low nibble 0, ends FF.
function IsReply([byte[]]$b) { foreach ($p in (Packets $b)) { if ($p.Length -ge 3 -and $p[0] -ge 0x90 -and ($p[0] -band 0x0F) -eq 0 -and $p[-1] -eq 0xFF) { return $true } }; $false }

function PanTilt([byte[]]$b) {
    foreach ($p in (Packets $b)) {
        if ($p.Length -eq 11 -and $p[1] -eq 0x50) {
            $pan = ([int]$p[2] -shl 12) -bor ([int]$p[3] -shl 8) -bor ([int]$p[4] -shl 4) -bor [int]$p[5]
            $tilt = ([int]$p[6] -shl 12) -bor ([int]$p[7] -shl 8) -bor ([int]$p[8] -shl 4) -bor [int]$p[9]
            $sp = if ($pan -ge 0x8000) { $pan - 0x10000 } else { $pan }
            $st = if ($tilt -ge 0x8000) { $tilt - 0x10000 } else { $tilt }
            return ("pan 0x{0:X4} ({1,6} = {2,7:N1} Lumens deg)  tilt 0x{3:X4} ({4,6} = {5,6:N1} Lumens deg)" -f $pan, $sp, ($sp / 160), $tilt, $st, ($st / 160))
        }
    }
    $null
}

function Inquiries($link, [int]$addr = 1) {
    $h = 0x80 + $addr
    $ok = $false
    foreach ($q in @(
            @('pan/tilt position', @($h, 0x09, 0x06, 0x12, 0xFF)),
            @('zoom position', @($h, 0x09, 0x04, 0x47, 0xFF)),
            @('version', @($h, 0x09, 0x00, 0x02, 0xFF)),
            @('power', @($h, 0x09, 0x04, 0x00, 0xFF)))) {
        $r = Xfer $link ([byte[]]$q[1]) 500
        Show "$($q[0]) (addr $addr)" $r
        if (IsReply $r.Bytes) { $ok = $true }
        $pt = PanTilt $r.Bytes; if ($pt) { Write-Host "      $pt" }
    }
    $ok
}

# 20 position inquiries back to back: reply rate and latency, which bound
# how fast calibration can poll.
function Reliability($link, [int]$addr = 1) {
    $h = 0x80 + $addr; $hits = 0; $times = @(); $script:quiet = $true
    for ($i = 0; $i -lt 20; $i++) {
        $r = Xfer $link ([byte[]]($h, 0x09, 0x06, 0x12, 0xFF)) 400
        if (PanTilt $r.Bytes) { $hits++; $times += $r.FirstMs }
    }
    $script:quiet = $false
    $avg = if ($times) { ($times | Measure-Object -Average -Maximum) } else { $null }
    if ($avg) { Write-Host ("   {0}/20 answered; first byte avg {1:N0} ms, max {2} ms" -f $hits, $avg.Average, $avg.Maximum) }
    else { Write-Host '   0/20 answered' }
}

# Drives like the calibrator does and logs every reply with a timestamp.
function MoveTest($link, [int]$addr = 1) {
    $h = 0x80 + $addr
    $inq = [byte[]]($h, 0x09, 0x06, 0x12, 0xFF)
    $zinq = [byte[]]($h, 0x09, 0x04, 0x47, 0xFF)
    function Poll($label, [int]$ms) {
        $end = $clock.ElapsedMilliseconds + $ms
        while ($clock.ElapsedMilliseconds -lt $end) {
            $r = Xfer $link $inq 150
            $pt = PanTilt $r.Bytes
            Write-Host ("   {0,7} ms {1,-12} {2}" -f $clock.ElapsedMilliseconds, $label, $(if ($pt) { $pt } else { "RX $(Hex $r.Bytes)" }))
        }
    }
    function Cmd($label, [byte[]]$c, [int]$ms = 300) { $r = Xfer $link $c $ms; Show $label $r }

    Write-Host '-- Home (ACK, then completion when it arrives?)'
    Cmd 'home' ([byte[]]($h, 0x01, 0x06, 0x04, 0xFF)) 6000
    Poll 'at home' 600
    Write-Host '-- Pan left slowly for 1.5 s (speed 05), polling position'
    Cmd 'drive left' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x01, 0x03, 0xFF))
    Poll 'left' 1500
    Cmd 'stop' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x03, 0xFF))
    Poll 'stopped' 600
    Write-Host '-- Pan right for 1.5 s'
    Cmd 'drive right' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x02, 0x03, 0xFF))
    Poll 'right' 1500
    Cmd 'stop' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x03, 0xFF))
    Write-Host '-- Tilt up for 1 s, then down for 1 s'
    Cmd 'drive up' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x01, 0xFF))
    Poll 'up' 1000
    Cmd 'drive down' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x02, 0xFF))
    Poll 'down' 1000
    Cmd 'stop' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x03, 0xFF))
    Write-Host '-- Pan and zoom drive back to back (the calibrator does this)'
    Cmd 'drive left' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x01, 0x03, 0xFF)) 50
    Cmd 'zoom tele' ([byte[]]($h, 0x01, 0x04, 0x07, 0x23, 0xFF))
    $end = $clock.ElapsedMilliseconds + 1500
    while ($clock.ElapsedMilliseconds -lt $end) {
        $a = Xfer $link $inq 150; $z = Xfer $link $zinq 150
        Write-Host ("   {0,7} ms {1}  zoom RX {2}" -f $clock.ElapsedMilliseconds, (PanTilt $a.Bytes), (Hex $z.Bytes))
    }
    Cmd 'stop' ([byte[]]($h, 0x01, 0x06, 0x01, 0x05, 0x05, 0x03, 0x03, 0xFF)) 100
    Cmd 'zoom stop' ([byte[]]($h, 0x01, 0x04, 0x07, 0x00, 0xFF))
    Write-Host '-- Absolute move to raw 0/0 (what `camera move <id> 0 0 0` should send), then back'
    Cmd 'abs 0,0' ([byte[]]($h, 0x01, 0x06, 0x02, 0x10, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF)) 4000
    Poll 'abs 0,0' 600
    Write-Host '-- Home + zoom wide (leaves it as calibration would)'
    Cmd 'home' ([byte[]]($h, 0x01, 0x06, 0x04, 0xFF)) 100
    Cmd 'zoom wide' ([byte[]]($h, 0x01, 0x04, 0x07, 0x37, 0xFF)) 4000
    Cmd 'zoom stop' ([byte[]]($h, 0x01, 0x04, 0x07, 0x00, 0xFF))
    Poll 'final' 600
}

$moveTarget = $null   # first link that answered, for -AllowMove

# ---------------------------------------------------------------------------
Section "Machine and network  ($(Get-Date -Format s))"
Write-Host "Computer: $env:COMPUTERNAME   PowerShell $($PSVersionTable.PSVersion)"
Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' } |
    Format-Table InterfaceAlias, IPAddress, PrefixLength, PrefixOrigin -AutoSize | Out-String -Width 200 | ForEach-Object { $_.Trim("`r`n") } | Write-Host
Get-NetConnectionProfile -ErrorAction SilentlyContinue | Format-Table InterfaceAlias, Name, NetworkCategory, IPv4Connectivity -AutoSize | Out-String -Width 200 | ForEach-Object { $_.Trim("`r`n") } | Write-Host
Get-NetRoute -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.DestinationPrefix -eq '0.0.0.0/0' } | Format-Table InterfaceAlias, NextHop, RouteMetric -AutoSize | Out-String -Width 200 | ForEach-Object { $_.Trim("`r`n") } | Write-Host
Write-Host '-- Wi-Fi'
netsh wlan show interfaces 2>$null | Select-String 'SSID|State|Signal|Radio' | ForEach-Object { Write-Host "   $($_.Line.Trim())" }
Write-Host '-- Neighbours (ARP); a camera shows up here once anything has talked to it'
Get-NetNeighbor -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object { $_.State -in 'Reachable', 'Stale', 'Delay', 'Probe' -and $_.LinkLayerAddress -and $_.LinkLayerAddress -ne 'FF-FF-FF-FF-FF-FF' } |
    Sort-Object IPAddress | Format-Table InterfaceAlias, IPAddress, LinkLayerAddress, State -AutoSize | Out-String -Width 200 | ForEach-Object { $_.Trim("`r`n") } | Write-Host

# ---------------------------------------------------------------------------
Section 'Sony-style discovery broadcast (UDP 52380); answers are a bonus'
foreach ($addr in (Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Where-Object { $_.IPAddress -notlike '127.*' -and $_.IPAddress -notlike '169.254.*' })) {
    $udp = $null
    try {
        $udp = New-Object Net.Sockets.UdpClient
        $udp.Client.SetSocketOption([Net.Sockets.SocketOptionLevel]::Socket, [Net.Sockets.SocketOptionName]::ReuseAddress, $true)
        $udp.Client.Bind((New-Object Net.IPEndPoint ([Net.IPAddress]::Parse($addr.IPAddress)), 52380))
        $udp.EnableBroadcast = $true
        $payload = [byte[]](0x02) + [Text.Encoding]::ASCII.GetBytes('ENQ:network') + [byte[]](0x03, 0xFF)
        [void]$udp.Send($payload, $payload.Length, (New-Object Net.IPEndPoint ([Net.IPAddress]::Broadcast), 52380))
        Write-Host "   sent from $($addr.IPAddress) ($($addr.InterfaceAlias))"
        $sw = [Diagnostics.Stopwatch]::StartNew()
        while ($sw.ElapsedMilliseconds -lt 2000) {
            if ($udp.Available -gt 0) {
                $f = New-Object Net.IPEndPoint ([Net.IPAddress]::Any), 0
                $d = $udp.Receive([ref]$f)
                if ($f.Address.ToString() -eq $addr.IPAddress) { continue }
                $text = -join ($d | ForEach-Object { if ($_ -ge 0x20 -and $_ -lt 0x7F) { [char]$_ } else { '.' } })
                Write-Host "   reply from ${f}: $text"
            } else { Start-Sleep -Milliseconds 5 }
        }
    } catch { Write-Host "   $($addr.IPAddress): $($_.Exception.Message)" }
    finally { if ($udp) { $udp.Close() } }
}

# ---------------------------------------------------------------------------
if ($CameraIp) {
    Section "Camera at $CameraIp"
    Write-Host ("   ping: {0}" -f (Test-Connection -ComputerName $CameraIp -Count 2 -Quiet -ErrorAction SilentlyContinue))
    foreach ($p in 80, 443, 554, 1259, 5678, 52381) {
        $c = New-Object Net.Sockets.TcpClient
        $open = $false
        try { $ar = $c.BeginConnect($CameraIp, $p, $null, $null); $open = $ar.AsyncWaitHandle.WaitOne(700) -and $c.Connected } catch {}
        $c.Close()
        Write-Host ("   TCP {0,-6} {1}" -f $p, $(if ($open) { 'open' } else { 'closed/filtered' }))
    }
    $n = Get-NetNeighbor -IPAddress $CameraIp -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($n) { Write-Host "   MAC: $($n.LinkLayerAddress) ($($n.State))" }

    Write-Host '-- VISCA-IP, with the Sony header (what PTZ Commander sends)'
    $ip = New-IpLink $CameraIp
    $ipOk = Inquiries $ip
    Write-Host '-- VISCA-IP, raw VISCA with no header (some cameras want this)'
    Show 'pan/tilt, raw' (Xfer $ip (Bytes '81 09 06 12 FF') 600 -raw)
    if ($ipOk) {
        Write-Host '-- 20 position inquiries'
        Reliability $ip
        $moveTarget = $ip
    } else { Close-Link $ip }
}

# ---------------------------------------------------------------------------
Section 'COM ports'
Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue | Where-Object { $_.Name -match '\(COM\d+\)' } |
    Format-Table Name, Manufacturer, PNPDeviceID -AutoSize | Out-String -Width 250 | ForEach-Object { $_.Trim("`r`n") } | Write-Host
$ports = if ($ComPort) { @($ComPort) } else { [IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique }
Write-Host "Probing: $($ports -join ', ')"

foreach ($port in $ports) {
    foreach ($baud in 9600, 38400) {
        Write-Host ''
        Write-Host "-- $port @ $baud 8N1"
        $link = $null
        try { $link = New-SerialLink $port $baud } catch { Write-Host "   can't open: $($_.Exception.Message)  (close PTZ Commander or anything else using it)"; break }
        try {
            # Address set: a working chain answers 88 30 0N FF (N = cameras + 1).
            Show 'address set (88 30 01 FF)' (Xfer $link (Bytes '88 30 01 FF') 600)
            Show 'IF_Clear (88 01 00 01 FF)' (Xfer $link (Bytes '88 01 00 01 FF') 600)
            $found = $null
            foreach ($a in 1..7) {
                $r = Xfer $link ([byte[]]((0x80 + $a), 0x09, 0x06, 0x12, 0xFF)) 400
                if ($a -eq 1 -or $r.Bytes.Length -gt 0) { Show "pan/tilt (addr $a)" $r }
                if (IsReply $r.Bytes) { $found = $a; break }
            }
            if ($found) {
                Write-Host "   ** answered at address $found **"
                [void](Inquiries $link $found)
                Write-Host '-- 20 position inquiries'
                Reliability $link $found
                if (-not $moveTarget) { $moveTarget = $link; $moveAddr = $found; $link = $null }
            }
        } finally { if ($link) { Close-Link $link } }
    }
}

# ---------------------------------------------------------------------------
if ($AllowMove) {
    if ($moveTarget) {
        Section "Movement test on $($moveTarget.Name)"
        $a = if ($moveAddr) { $moveAddr } else { 1 }
        MoveTest $moveTarget $a
    } else {
        Section 'Movement test skipped: nothing answered'
    }
}
if ($moveTarget) { Close-Link $moveTarget }

Section "Done in $([Math]::Round($clock.Elapsed.TotalSeconds, 1)) s"
Write-Host "Log: $log"
Stop-Transcript | Out-Null
