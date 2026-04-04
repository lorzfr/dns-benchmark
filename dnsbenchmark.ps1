#Requires -Version 5.1
<#
.SYNOPSIS
    DNS Resolver Benchmark — raw UDP sockets, PS 5.1 compatible

.DESCRIPTION
    Tests DNS servers for latency and reliability using raw UDP packets.
    Pass -Servers to test specific servers; omit it to run the full built-in list.
    GNU-style flags like --servers and --rounds are also accepted for convenience.

.PARAMETER Servers
    One or more DNS servers to test.
    Format:  "IP"  or  "IP:Port"  (default port is 53)
    Example: .\DNS-Benchmark.ps1 -Servers "1.1.1.1","8.8.4.4","8.8.8.8"

.PARAMETER Rounds
    How many times each domain is queried per server. Default: 3

.PARAMETER TimeoutMs
    Per-query timeout in milliseconds. Default: 1500

.PARAMETER ExportCsv
    Save raw results to a timestamped CSV next to the script.

.EXAMPLE
    # Full built-in list
    .\DNS-Benchmark.ps1

.EXAMPLE
    # Only test Cloudflare and Google
    .\DNS-Benchmark.ps1 -Servers "1.1.1.1","8.8.8.8"

.EXAMPLE
    # GNU-style flags work too
    .\DNS-Benchmark.ps1 --servers "1.1.1.1","8.8.4.4" --rounds 5

.EXAMPLE
    # 5 rounds, 2 s timeout, export CSV
    .\DNS-Benchmark.ps1 -Rounds 5 -TimeoutMs 2000 -ExportCsv
#>

[CmdletBinding(PositionalBinding = $false)]
param(
    [string[]] $Servers    = @(),
    [int]      $Rounds     = 3,
    [int]      $TimeoutMs  = 1500,
    [switch]   $ExportCsv,
    [Parameter(ValueFromRemainingArguments = $true)]
    [object[]] $RemainingArgs
)

function Expand-ArgumentTokens {
    param([object[]]$Items)

    $tokens = New-Object System.Collections.Generic.List[string]

    foreach ($item in $Items) {
        if ($null -eq $item) { continue }

        if (($item -is [System.Array]) -and -not ($item -is [string])) {
            foreach ($nested in (Expand-ArgumentTokens -Items $item)) {
                $tokens.Add($nested)
            }
            continue
        }

        $tokens.Add([string]$item)
    }

    return $tokens.ToArray()
}

function Parse-CompatibilityArguments {
    param(
        [string[]]$Args,
        [string[]]$InitialServers,
        [int]$InitialRounds,
        [int]$InitialTimeoutMs,
        [bool]$InitialExportCsv
    )

    $parsedServers = New-Object System.Collections.Generic.List[string]
    foreach ($server in $InitialServers) {
        $parsedServers.Add($server)
    }

    $parsedRounds    = $InitialRounds
    $parsedTimeoutMs = $InitialTimeoutMs
    $parsedExportCsv = $InitialExportCsv
    $hasServerOverride = $false

    $index = 0
    while ($index -lt $Args.Count) {
        $token = $Args[$index]

        switch -Regex ($token) {
            '^--servers$' {
                $index++
                if ($index -ge $Args.Count -or $Args[$index] -match '^--') {
                    throw "The --servers flag requires at least one IP or IP:Port value."
                }

                if (-not $hasServerOverride) {
                    $parsedServers.Clear()
                    $hasServerOverride = $true
                }

                while ($index -lt $Args.Count -and $Args[$index] -notmatch '^--') {
                    $parsedServers.Add($Args[$index])
                    $index++
                }

                continue
            }

            '^--servers=(.+)$' {
                $values = @($Matches[1] -split ',' | ForEach-Object { $_.Trim() } | Where-Object { $_ })
                if ($values.Count -eq 0) {
                    throw "The --servers flag requires at least one IP or IP:Port value."
                }

                if (-not $hasServerOverride) {
                    $parsedServers.Clear()
                    $hasServerOverride = $true
                }

                foreach ($value in $values) {
                    $parsedServers.Add($value)
                }

                $index++
                continue
            }

            '^--rounds$' {
                if ($index + 1 -ge $Args.Count) {
                    throw "The --rounds flag requires an integer value."
                }

                $parsedRounds = [int]$Args[$index + 1]
                $index += 2
                continue
            }

            '^--rounds=(.+)$' {
                $parsedRounds = [int]$Matches[1]
                $index++
                continue
            }

            '^--timeout(?:-ms|ms)?$' {
                if ($index + 1 -ge $Args.Count) {
                    throw "The --timeout-ms flag requires an integer value in milliseconds."
                }

                $parsedTimeoutMs = [int]$Args[$index + 1]
                $index += 2
                continue
            }

            '^--timeout(?:-ms|ms)?=(.+)$' {
                $parsedTimeoutMs = [int]$Matches[1]
                $index++
                continue
            }

            '^--export(?:-csv|csv)?$' {
                $parsedExportCsv = $true
                $index++
                continue
            }

            default {
                throw "Unrecognized argument '$token'. Use PowerShell syntax like -Servers or GNU-style flags like --servers."
            }
        }
    }

    return [PSCustomObject]@{
        Servers           = $parsedServers.ToArray()
        Rounds            = $parsedRounds
        TimeoutMs         = $parsedTimeoutMs
        ExportCsv         = $parsedExportCsv
        HasServerOverride = $hasServerOverride
    }
}

$compatArgs = Expand-ArgumentTokens -Items $RemainingArgs
if ($compatArgs.Count -gt 0) {
    $compat = Parse-CompatibilityArguments `
        -Args $compatArgs `
        -InitialServers $Servers `
        -InitialRounds $Rounds `
        -InitialTimeoutMs $TimeoutMs `
        -InitialExportCsv ([bool]$ExportCsv)

    if ($compat.HasServerOverride) {
        $Servers = $compat.Servers
    }

    $Rounds    = $compat.Rounds
    $TimeoutMs = $compat.TimeoutMs
    $ExportCsv = [switch]$compat.ExportCsv
}

# ─────────────────────────────────────────────
#  BUILT-IN SERVER LIST  (used when -Servers is not supplied)
# ─────────────────────────────────────────────
$BuiltInServers = @(
    @{ Name = "Google Primary"       ; IP = "8.8.8.8"        ; Port = 53   }
    @{ Name = "Google Secondary"     ; IP = "8.8.4.4"        ; Port = 53   }
    @{ Name = "Cloudflare Primary"   ; IP = "1.1.1.1"        ; Port = 53   }
    @{ Name = "Cloudflare Secondary" ; IP = "1.0.0.1"        ; Port = 53   }
    @{ Name = "Quad9 (filtered)"     ; IP = "9.9.9.9"        ; Port = 53   }
    @{ Name = "Quad9 (unfiltered)"   ; IP = "9.9.9.10"       ; Port = 53   }
    @{ Name = "OpenDNS Primary"      ; IP = "208.67.222.222"  ; Port = 53   }
    @{ Name = "OpenDNS Secondary"    ; IP = "208.67.220.220"  ; Port = 53   }
    @{ Name = "AdGuard"              ; IP = "94.140.14.14"   ; Port = 53   }
)

# ─────────────────────────────────────────────
#  KNOWN-IP  ->  FRIENDLY NAME  lookup
# ─────────────────────────────────────────────
$KnownNames = @{
    "8.8.8.8"         = "Google Primary"
    "8.8.4.4"         = "Google Secondary"
    "1.1.1.1"         = "Cloudflare Primary"
    "1.0.0.1"         = "Cloudflare Secondary"
    "9.9.9.9"         = "Quad9 (filtered)"
    "9.9.9.10"        = "Quad9 (unfiltered)"
    "208.67.222.222"  = "OpenDNS Primary"
    "208.67.220.220"  = "OpenDNS Secondary"
    "94.140.14.14"    = "AdGuard"
    "94.140.15.15"    = "AdGuard Secondary"
}

# ─────────────────────────────────────────────
#  Parse "IP" or "IP:Port" string into a server hashtable
# ─────────────────────────────────────────────
function ConvertTo-ServerEntry {
    param([string]$Raw)

    $Raw = $Raw.Trim()

    if ($Raw -match '^(.+):(\d+)$') {
        $ip   = $Matches[1].Trim()
        $port = [int]$Matches[2]
    } else {
        $ip   = $Raw
        $port = 53
    }

    # Friendly name: use known table, or fall back to the raw string
    if ($KnownNames.ContainsKey($ip)) {
        $name = $KnownNames[$ip]
        if ($port -ne 53) { $name = "$name :$port" }
    } else {
        $name = if ($port -ne 53) { "$ip`:$port" } else { $ip }
    }

    return @{ Name = $name ; IP = $ip ; Port = $port }
}

# ─────────────────────────────────────────────
#  Resolve active server list
# ─────────────────────────────────────────────
if ($Servers.Count -gt 0) {
    $DnsServers = @(foreach ($raw in $Servers) {
        ConvertTo-ServerEntry -Raw $raw
    })
    $Mode = "Custom  (-Servers / --servers)"
} else {
    $DnsServers = $BuiltInServers
    $Mode = "Built-in list (no -Servers flag given)"
}

# ─────────────────────────────────────────────
#  TEST DOMAINS
# ─────────────────────────────────────────────
$TestDomains = @(
    "google.com"
    "youtube.com"
    "facebook.com"
    "amazon.com"
    "github.com"
    "microsoft.com"
    "cloudflare.com"
    "reddit.com"
    "wikipedia.org"
    "stackoverflow.com"
)

# ─────────────────────────────────────────────
#  Build a minimal RFC-1035 DNS A-query packet
# ─────────────────────────────────────────────
function New-DnsQueryPacket {
    param([string]$Domain)

    $stream = New-Object System.IO.MemoryStream

    $txId = [byte[]]@((Get-Random -Maximum 256), (Get-Random -Maximum 256))
    $stream.Write($txId, 0, 2)

    # Flags: standard recursive query
    $stream.Write([byte[]]@(0x01, 0x00), 0, 2)

    # QDCOUNT=1, rest 0
    $stream.Write([byte[]]@(0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00), 0, 8)

    # QNAME
    foreach ($label in $Domain.Split('.')) {
        $b = [System.Text.Encoding]::ASCII.GetBytes($label)
        $stream.WriteByte([byte]$b.Length)
        $stream.Write($b, 0, $b.Length)
    }
    $stream.WriteByte([byte]0)

    # QTYPE=A, QCLASS=IN
    $stream.Write([byte[]]@(0x00, 0x01, 0x00, 0x01), 0, 4)

    return @{ Packet = $stream.ToArray() ; TxId = $txId }
}

# ─────────────────────────────────────────────
#  Send query — returns elapsed ms or $null
# ─────────────────────────────────────────────
function Invoke-DnsQuery {
    param(
        [string] $IP,
        [int]    $Port,
        [string] $Domain,
        [int]    $TimeoutMs
    )

    $q      = New-DnsQueryPacket -Domain $Domain
    $packet = $q.Packet
    $txId   = $q.TxId
    $udp    = $null
    $sw     = [System.Diagnostics.Stopwatch]::StartNew()

    try {
        $udp = New-Object System.Net.Sockets.UdpClient
        $udp.Client.ReceiveTimeout = $TimeoutMs
        $udp.Connect($IP, $Port)
        [void]$udp.Send($packet, $packet.Length)

        $ep  = New-Object System.Net.IPEndPoint([System.Net.IPAddress]::Any, 0)
        $rsp = $udp.Receive([ref]$ep)
        $sw.Stop()

        if ($rsp.Length -lt 4)                                     { return $null }
        if ($rsp[0] -ne $txId[0] -or $rsp[1] -ne $txId[1])       { return $null }

        $rcode = $rsp[3] -band 0x0F
        if ($rcode -gt 3)                                           { return $null }

        return [math]::Round($sw.Elapsed.TotalMilliseconds, 1)
    }
    catch {
        return $null
    }
    finally {
        if ($sw.IsRunning) { $sw.Stop() }
        if ($null -ne $udp) { try { $udp.Close() } catch {} }
    }
}

# ─────────────────────────────────────────────
#  Display helpers
# ─────────────────────────────────────────────
function Write-Header {
    param([string]$Text)
    $line = "=" * 72
    Write-Host ""
    Write-Host $line               -ForegroundColor Cyan
    Write-Host ("  {0}" -f $Text) -ForegroundColor Cyan
    Write-Host $line               -ForegroundColor Cyan
}

function Write-Section {
    param([string]$Text)
    Write-Host ""
    Write-Host ("  -- {0}" -f $Text) -ForegroundColor Yellow
}

function Get-Stats {
    param([double[]]$Values)

    if (-not $Values -or $Values.Count -eq 0) {
        return @{ Min=$null; Max=$null; Avg=$null; Median=$null; StdDev=$null; Jitter=$null }
    }

    $sorted = $Values | Sort-Object
    $n      = $sorted.Count
    $avg    = ($sorted | Measure-Object -Average).Average

    if ($n % 2 -eq 0) {
        $median = ($sorted[($n / 2) - 1] + $sorted[$n / 2]) / 2
    } else {
        $median = $sorted[[math]::Floor($n / 2)]
    }

    $sumSq  = ($sorted | ForEach-Object { [math]::Pow($_ - $avg, 2) } | Measure-Object -Sum).Sum
    $stddev = [math]::Sqrt($sumSq / $n)

    if ($n -gt 1) {
        $diffs  = for ($i = 1; $i -lt $n; $i++) { [math]::Abs($sorted[$i] - $sorted[$i - 1]) }
        $jitter = ($diffs | Measure-Object -Average).Average
    } else {
        $jitter = 0
    }

    return @{
        Min    = [math]::Round($sorted[0],  1)
        Max    = [math]::Round($sorted[-1], 1)
        Avg    = [math]::Round($avg,        1)
        Median = [math]::Round($median,     1)
        StdDev = [math]::Round($stddev,     1)
        Jitter = [math]::Round($jitter,     1)
    }
}

function Format-Ms {
    param($Value)
    if ($null -eq $Value) { return "   FAIL " }
    return ("{0,7:F1} ms" -f $Value)
}

function Get-LatencyColor {
    param($Ms)
    if ($null -eq $Ms) { return "Red"        }
    if ($Ms -lt 10)    { return "Magenta"    }
    if ($Ms -lt 30)    { return "Green"      }
    if ($Ms -lt 80)    { return "Yellow"     }
    if ($Ms -lt 150)   { return "DarkYellow" }
    return "Red"
}

function Format-Cell {
    param($Value, $Unit)
    if ($null -ne $Value) { return "$Value $Unit" }
    return "FAIL"
}

# ─────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────

Clear-Host
Write-Header "DNS RESOLVER BENCHMARK  --  PowerShell $($PSVersionTable.PSVersion)"
Write-Host ""
Write-Host "  Mode     : $Mode"                                        -ForegroundColor White
Write-Host "  Servers  : $($DnsServers.Count)"                        -ForegroundColor White
Write-Host "  Domains  : $($TestDomains.Count)"                       -ForegroundColor White
Write-Host "  Rounds   : $Rounds"                                      -ForegroundColor White
Write-Host "  Timeout  : $TimeoutMs ms"                                -ForegroundColor White
Write-Host "  Started  : $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"   -ForegroundColor White

# Print active server list
Write-Host ""
Write-Host "  Servers to test:" -ForegroundColor White
foreach ($srv in $DnsServers) {
    $tag = if ($srv.Port -ne 53) { " (port $($srv.Port))" } else { "" }
    Write-Host ("    {0,-28} {1}{2}" -f $srv.Name, $srv.IP, $tag) -ForegroundColor DarkCyan
}

# Initialise storage
$results    = @{}
$allRawRows = @()
foreach ($srv in $DnsServers) {
    $results[$srv.Name] = @{}
    foreach ($d in $TestDomains) {
        $results[$srv.Name][$d] = @()
    }
}

# ── Benchmark rounds ──────────────────────────
for ($round = 1; $round -le $Rounds; $round++) {

    Write-Section "Round $round / $Rounds"

    foreach ($srv in $DnsServers) {
        $name = $srv.Name
        $ip   = $srv.IP
        $port = $srv.Port

        Write-Host ("    {0,-28} ({1}:{2,-5})  " -f $name, $ip, $port) -NoNewline

        $roundTotal = 0
        $roundFails = 0

        foreach ($domain in $TestDomains) {
            $ms = Invoke-DnsQuery -IP $ip -Port $port -Domain $domain -TimeoutMs $TimeoutMs

            if ($null -ne $ms) {
                $results[$name][$domain] += $ms
                $roundTotal += $ms
            } else {
                $roundFails++
            }

            if ($ExportCsv) {
                $allRawRows += [PSCustomObject]@{
                    Round     = $round
                    Server    = $name
                    IP        = $ip
                    Port      = $port
                    Domain    = $domain
                    LatencyMs = if ($null -ne $ms) { $ms } else { "FAIL" }
                }
            }
        }

        $okCount = $TestDomains.Count - $roundFails
        if ($okCount -gt 0) {
            $avgRound = [math]::Round($roundTotal / $okCount, 1)
        } else {
            $avgRound = $null
        }

        $color = Get-LatencyColor -Ms $avgRound
        Write-Host (Format-Ms -Value $avgRound) -ForegroundColor $color -NoNewline

        if ($roundFails -gt 0) {
            Write-Host ("  ($roundFails fail)") -ForegroundColor DarkRed
        } else {
            Write-Host ""
        }
    }
}

# ── Results summary ───────────────────────────
Write-Header "RESULTS SUMMARY  (all rounds combined)"

$summary = foreach ($srv in $DnsServers) {
    $name  = $srv.Name
    $allMs = @()
    $fails = 0

    foreach ($d in $TestDomains) {
        $v = $results[$name][$d]
        if ($v.Count -gt 0) { $allMs += $v } else { $fails++ }
    }

    $st  = Get-Stats -Values $allMs
    $pct = [math]::Round(($allMs.Count / ($TestDomains.Count * $Rounds)) * 100, 1)

    [PSCustomObject]@{
        Rank        = 0
        Name        = $name
        Endpoint    = "$($srv.IP):$($srv.Port)"
        Avg_ms      = $st.Avg
        Median_ms   = $st.Median
        Min_ms      = $st.Min
        Max_ms      = $st.Max
        StdDev_ms   = $st.StdDev
        Jitter_ms   = $st.Jitter
        SuccessRate = $pct
    }
}

$ranked = $summary | Sort-Object {
    if ($null -eq $_.Median_ms) { 99999 } else { $_.Median_ms }
}

$rank = 1
foreach ($x in $ranked) { $x.Rank = $rank++ }

$hdr = "{0,-4} {1,-28} {2,-22} {3,9} {4,9} {5,7} {6,7} {7,8} {8,8} {9,8}" -f `
       "Rank","Server","Endpoint","Avg","Median","Min","Max","StdDev","Jitter","Success%"
$div = "-" * 108

Write-Host ""
Write-Host $hdr -ForegroundColor White
Write-Host $div -ForegroundColor DarkGray

foreach ($x in $ranked) {
    $color = Get-LatencyColor -Ms $x.Avg_ms
    $line  = "{0,-4} {1,-28} {2,-22} {3,9} {4,9} {5,7} {6,7} {7,8} {8,8} {9,8}" -f `
             "#$($x.Rank)",
             $x.Name,
             $x.Endpoint,
             (Format-Cell -Value $x.Avg_ms    -Unit "ms"),
             (Format-Cell -Value $x.Median_ms -Unit "ms"),
             (Format-Cell -Value $x.Min_ms    -Unit "ms"),
             (Format-Cell -Value $x.Max_ms    -Unit "ms"),
             (Format-Cell -Value $x.StdDev_ms -Unit "ms"),
             (Format-Cell -Value $x.Jitter_ms -Unit "ms"),
             "$($x.SuccessRate)%"
    Write-Host $line -ForegroundColor $color
}

Write-Host $div -ForegroundColor DarkGray
Write-Host ""
Write-Host "  Latency legend:" -ForegroundColor White
Write-Host "    " -NoNewline; Write-Host "< 10 ms   Outstanding            " -ForegroundColor Magenta
Write-Host "    " -NoNewline; Write-Host "< 30 ms   Excellent              " -ForegroundColor Green
Write-Host "    " -NoNewline; Write-Host "30-79 ms  Good                   " -ForegroundColor Yellow
Write-Host "    " -NoNewline; Write-Host "80-149 ms Fair                   " -ForegroundColor DarkYellow
Write-Host "    " -NoNewline; Write-Host ">= 150 ms Poor                   " -ForegroundColor Red

# ── Per-domain breakdown ──────────────────────
Write-Header "PER-DOMAIN BREAKDOWN  (median ms -- green = fastest)"

$dw = 22
$cw = 12
$hdrRow = "{0,-$dw}" -f "Domain"
foreach ($srv in $DnsServers) {
    $short = $srv.Name `
        -replace " Primary",   "" `
        -replace " Secondary", " 2nd" `
        -replace "\(.*\)",     ""
    $short = $short.Trim()
    if ($short.Length -gt ($cw - 1)) { $short = $short.Substring(0, $cw - 1) }
    $hdrRow += ("{0,$cw}" -f $short)
}
Write-Host ""
Write-Host $hdrRow -ForegroundColor White
Write-Host ("-" * ($dw + $DnsServers.Count * $cw)) -ForegroundColor DarkGray

foreach ($domain in $TestDomains) {
    $bestMed = $null
    foreach ($srv in $DnsServers) {
        $v = $results[$srv.Name][$domain]
        if ($v.Count -gt 0) {
            $m = (Get-Stats -Values $v).Median
            if ($null -eq $bestMed -or $m -lt $bestMed) { $bestMed = $m }
        }
    }

    Write-Host ("{0,-$dw}" -f $domain) -NoNewline -ForegroundColor White

    foreach ($srv in $DnsServers) {
        $v = $results[$srv.Name][$domain]
        if ($v.Count -gt 0) {
            $m     = (Get-Stats -Values $v).Median
            $color = if ($m -eq $bestMed) { "Green" } else { Get-LatencyColor -Ms $m }
            Write-Host ("{0,$cw}" -f "$m ms") -NoNewline -ForegroundColor $color
        } else {
            Write-Host ("{0,$cw}" -f "FAIL") -NoNewline -ForegroundColor DarkRed
        }
    }
    Write-Host ""
}

# ── Winner ────────────────────────────────────
Write-Header "WINNER"
$winner = $ranked | Select-Object -First 1
$wColor = Get-LatencyColor -Ms $winner.Median_ms
Write-Host ""
Write-Host ("  ## {0}  ({1})" -f $winner.Name, $winner.Endpoint) -ForegroundColor $wColor
Write-Host ("     Median {0} ms  |  Avg {1} ms  |  Success {2}%" -f `
            $winner.Median_ms, $winner.Avg_ms, $winner.SuccessRate) -ForegroundColor $wColor
Write-Host ""

# ── Optional CSV export ───────────────────────
if ($ExportCsv) {
    $csvPath = Join-Path $PSScriptRoot "DNS-Benchmark-$(Get-Date -Format 'yyyyMMdd-HHmmss').csv"
    $allRawRows | Export-Csv -Path $csvPath -NoTypeInformation -Encoding UTF8
    Write-Host "  Exported: $csvPath" -ForegroundColor Cyan
    Write-Host ""
}

Write-Host "  Finished: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')" -ForegroundColor DarkGray
Write-Host ""
