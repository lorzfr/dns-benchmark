# DNS Benchmark

A native C++17 DNS benchmarking CLI, built as `dnsbenchmark`.

## What it does

The tool benchmarks one or more DNS resolvers against a fixed set of common domains.

For each resolver it:

- sends direct UDP DNS A-record queries
- measures latency in milliseconds
- counts failures/timeouts
- calculates avg, median, min, max, standard deviation, and jitter
- prints ranked summary tables and a winner
- optionally exports raw measurements to CSV

## Requirements

- Linux (Ubuntu/Debian is the primary target)
- CMake 3.16+
- C++17 compiler
- Network access to tested DNS resolvers

## Install (Debian/Ubuntu)

```bash
./install-debian.sh
```

Optional install prefix:

```bash
INSTALL_PREFIX=/opt/dnsbenchmark ./install-debian.sh
```

## Build and test

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/dnsbenchmark
```

## CLI usage

```bash
./build/dnsbenchmark [--servers <server...>] [--server <server>] [--rounds <int>] [--timeout <int>] [--timeout-ms <int>] [--export-csv] [--tui]
```

Examples:

```bash
./build/dnsbenchmark
./build/dnsbenchmark --servers 1.1.1.1 8.8.8.8 --rounds 5 --timeout 2000 --export-csv
./build/dnsbenchmark --server 192.168.178.201
```

## TUI mode

```bash
./build/dnsbenchmark --tui
```

## Parameters

- `--servers`: one or more resolvers (`IP` or `IP:PORT`)
- `--server`: single resolver shorthand
- `--rounds`: benchmark passes per resolver (default: `3`)
- `--timeout` / `--timeout-ms`: per-query timeout in ms (default: `1500`)
- `--export-csv`: write timestamped CSV output

CSV format:

- `DNS-Benchmark-YYYYMMDD-HHMMSS.csv`

## Built-in test domains

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

## Built-in resolver list

- `8.8.8.8` (`Google Primary`)
- `8.8.4.4` (`Google Secondary`)
- `1.1.1.1` (`Cloudflare Primary`)
- `1.0.0.1` (`Cloudflare Secondary`)
- `9.9.9.9` (`Quad9 (filtered)`)
- `9.9.9.10` (`Quad9 (unfiltered)`)
- `208.67.222.222` (`OpenDNS Primary`)
- `208.67.220.220` (`OpenDNS Secondary`)
- `94.140.14.14` (`AdGuard`)

## Output sections

- `DNS RESOLVER BENCHMARK`: start banner + settings
- `Round X / Y`: live round progress
- `RESULTS SUMMARY`: ranked resolver table
- `PER-DOMAIN BREAKDOWN`: best resolver by domain median
- `WINNER`: overall best resolver by median latency

## Project layout

- Native headers: `include/dnsbenchmark/`
- Native sources: `src/`
- Tests: `tests/`
- Legacy/archived scripts: `scripts-old/`
