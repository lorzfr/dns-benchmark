# DNS Benchmark

DNS benchmark project with a native C++17 CLI built as `dnsbenchmark`.

This project is entirely vibe-coded with Anthropic Claude Sonnet 4.6 and OpenAI GPT-5.4.

## What It Does

The CLI tests one or more DNS resolvers against a fixed list of common domains.

For each resolver, the benchmark:

- sends direct UDP DNS A-record queries
- measures response time in milliseconds
- counts failures and timeouts
- calculates average, median, min, max, standard deviation, and jitter
- shows a ranked summary, a per-domain breakdown, and a final winner
- can export all raw measurements to CSV

## Requirements

### Native Linux CLI

- CMake 3.16 or newer
- A C++17 compiler
- Ubuntu/Debian x64 is the primary target for v1
- Network access to the DNS servers you want to test

## Quick Install Scripts

### Debian / Ubuntu

Use the automated installer to install dependencies, build the native CLI, and install it to `/usr/local`:

```bash
./install-debian.sh
```

Optional install prefix:

```bash
INSTALL_PREFIX=/opt/dnsbenchmark ./install-debian.sh
```

## Project Files

- Archived scripts: `./scripts-old/`
- Native build entrypoint: `.\CMakeLists.txt`
- Native headers: `.\include\dnsbenchmark\`
- Native sources: `.\src\`
- Native tests: `.\tests\`

## Syntax

### Native Linux CLI

```bash
./dnsbenchmark [--servers <server...>] [--rounds <int>] [--timeout <int>] [--timeout-ms <int>] [--export-csv] [--tui]
```

For a single custom resolver, you can also use:

```bash
./dnsbenchmark --server 192.168.178.201
```

### Interactive TUI mode

Launch a guided terminal UI for easy setup:

```bash
./build/dnsbenchmark --tui
```

The TUI uses a restrained color palette and adapts to terminal widths automatically.

## Build On Linux

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Built executable:

```bash
./build/dnsbenchmark
```

## Build A Debian Package

After configuring and building on Ubuntu/Debian:

```bash
cmake -S . -B build
cmake --build build
cmake --build build --target package_deb
```

Alternative:

```bash
cpack --config build/CPackConfig.cmake -G DEB
```

The generated `.deb` file will be written into `./build/`.

Install it with:

```bash
sudo apt install ./build/dnsbenchmark_<version>_<arch>.deb
```

## Parameters

### `Servers`

One or more DNS servers to test.

Accepted formats:

- `IP`
- `IP:Port`

Examples:

- `1.1.1.1`
- `8.8.8.8`
- `192.168.178.201:5335`

If no servers are provided, the benchmark uses its built-in public resolver list.

### `Rounds`

How many benchmark passes to run for each server.

Default:

- `3`

### `TimeoutMs`

Per-query timeout in milliseconds.

Default:

- `1500`

### `ExportCsv`

If present, the benchmark writes all raw results to a CSV file in the executable folder.

Filename format:

- `DNS-Benchmark-YYYYMMDD-HHMMSS.csv`

## Examples

### Native Linux binary with built-in servers

```bash
./build/dnsbenchmark
```

### Native Linux binary with custom servers

```bash
./build/dnsbenchmark --servers 1.1.1.1 8.8.4.4 --rounds 5 --timeout 2000 --export-csv
```

### Native Linux binary with one custom server

```bash
./build/dnsbenchmark --server 192.168.178.201
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

If you do not pass custom servers, the benchmark currently tests these public resolvers:

- `8.8.8.8` as `Google Primary`
- `8.8.4.4` as `Google Secondary`
- `1.1.1.1` as `Cloudflare Primary`
- `1.0.0.1` as `Cloudflare Secondary`
- `9.9.9.9` as `Quad9 (filtered)`
- `9.9.9.10` as `Quad9 (unfiltered)`
- `208.67.222.222` as `OpenDNS Primary`
- `208.67.220.220` as `OpenDNS Secondary`
- `94.140.14.14` as `AdGuard`

## How It Works

1. It builds the active DNS server list from either the built-in list or your `Servers` input.
2. For each round, it sends raw UDP DNS A-record queries to every server for every test domain.
3. Each query is timed with a stopwatch or steady clock.
4. A reply is accepted only if the response looks valid and matches the request transaction ID.
5. Successful timings are stored; failures are counted as `FAIL`.
6. After all rounds finish, the benchmark calculates summary stats and ranks servers by median latency.
7. It prints a round-by-round live view, a final results summary, a per-domain median breakdown, and the winning resolver.
8. If CSV export is enabled, it writes all raw rows to a timestamped file.

## Output Sections

When you run either implementation, you will see:

- `DNS RESOLVER BENCHMARK`: start banner and current settings
- `Round X / Y`: live progress for each round
- `RESULTS SUMMARY`: ranked resolver table
- `PER-DOMAIN BREAKDOWN`: best median per domain
- `WINNER`: best overall resolver based on median latency

## Notes

- Both implementations use raw UDP DNS queries, not `Resolve-DnsName` or `nslookup`.
- Success rate is based on successful replies across all tested domains and rounds.
- A lower median is generally the most useful value for comparing resolvers.
- The native Linux CLI is the future canonical implementation shape for cross-platform work.
- 