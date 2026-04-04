# DNS Benchmark

Small PowerShell DNS benchmark that sends raw UDP DNS queries, measures latency, and compares resolver reliability.

This script is entirely vibe-coded with Anthropic Claude Sonnet 4.6 and OpenAI GPT-5.4.

## What It Does

`dnsbenchmark.ps1` tests one or more DNS resolvers against a fixed list of common domains.

For each resolver, it:

- sends direct UDP DNS A-record queries
- measures response time in milliseconds
- counts failures and timeouts
- calculates average, median, min, max, standard deviation, and jitter
- shows a ranked summary, a per-domain breakdown, and a final winner
- can export all raw measurements to CSV

## Requirements

- Windows PowerShell 5.1 or newer
- Network access to the DNS servers you want to test
- UDP access to port `53` or the custom DNS port you specify

## Script File

- Main script: `.\dnsbenchmark.ps1`

## Syntax

### PowerShell-style syntax

```powershell
.\dnsbenchmark.ps1 [-Servers <string[]>] [-Rounds <int>] [-TimeoutMs <int>] [-ExportCsv]
```

### GNU-style compatibility syntax

```powershell
.\dnsbenchmark.ps1 [--servers <server...>] [--rounds <int>] [--timeout <int>] [--timeout-ms <int>] [--export-csv]
```

## Parameters

### `-Servers`

One or more DNS servers to test.

Accepted formats:

- `IP`
- `IP:Port`

Examples:

- `"1.1.1.1"`
- `"8.8.8.8"`
- `"192.168.178.201:5335"`

If `-Servers` is omitted, the script uses its built-in public resolver list.

### `-Rounds`

How many benchmark passes to run for each server.

Default:

- `3`

### `-TimeoutMs`

Per-query timeout in milliseconds.

Default:

- `1500`

### `-ExportCsv`

If present, the script writes all raw results to a CSV file in the same folder as the script.

Filename format:

- `DNS-Benchmark-YYYYMMDD-HHMMSS.csv`

## Examples

### Use the built-in server list

```powershell
.\dnsbenchmark.ps1
```

### Test specific servers

```powershell
.\dnsbenchmark.ps1 -Servers "1.1.1.1","8.8.4.4","8.8.8.8"
```

### Test a custom DNS port

```powershell
.\dnsbenchmark.ps1 -Servers "192.168.178.201:5335","1.1.1.1"
```

### Run more rounds with a longer timeout

```powershell
.\dnsbenchmark.ps1 -Rounds 5 -TimeoutMs 2000
```

### Export results to CSV

```powershell
.\dnsbenchmark.ps1 -ExportCsv
```

### GNU-style example

```powershell
.\dnsbenchmark.ps1 --servers "1.1.1.1","8.8.4.4" --rounds 5 --timeout 2000 --export-csv
```

## Built-In Test Domains

The benchmark currently tests these domains:

- `google.com`
- `youtube.com`
- `facebook.com`
- `amazon.com`
- `github.com`
- `microsoft.com`
- `cloudflare.com`
- `reddit.com`
- `wikipedia.org`
- `stackoverflow.com`

## Built-In Resolver List

If you do not pass `-Servers`, the script currently benchmarks these public resolvers:

- `8.8.8.8` as `Google Primary`
- `8.8.4.4` as `Google Secondary`
- `1.1.1.1` as `Cloudflare Primary`
- `1.0.0.1` as `Cloudflare Secondary`
- `9.9.9.9` as `Quad9 (filtered)`
- `9.9.9.10` as `Quad9 (unfiltered)`
- `208.67.222.222` as `OpenDNS Primary`
- `208.67.220.220` as `OpenDNS Secondary`
- `94.140.14.14` as `AdGuard`

## How The Script Works

1. It builds the active DNS server list from either the built-in list or your `-Servers` / `--servers` input.
2. For each round, it sends raw UDP DNS A-record queries to every server for every test domain.
3. Each query is timed with a stopwatch.
4. A reply is accepted only if the response looks valid and matches the request transaction ID.
5. Successful timings are stored; failures are counted as `FAIL`.
6. After all rounds finish, the script calculates summary stats and ranks the servers by median latency.
7. It prints a round-by-round live view, a final results summary, a per-domain median breakdown, and the winning resolver.
8. If CSV export is enabled, it writes all raw rows to a timestamped file.

## Output Sections

When you run the script, you will see:

- `DNS RESOLVER BENCHMARK`: start banner and current settings
- `Round X / Y`: live progress for each round
- `RESULTS SUMMARY`: ranked resolver table
- `PER-DOMAIN BREAKDOWN`: best median per domain
- `WINNER`: best overall resolver based on median latency

## Notes

- The script uses raw UDP DNS queries, not `Resolve-DnsName` or `nslookup`.
- Success rate is based on successful replies across all tested domains and rounds.
- A lower median is generally the most useful value for comparing resolvers.
- If script execution is blocked on your machine, you can launch it with:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\dnsbenchmark.ps1
```
