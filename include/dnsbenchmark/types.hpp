#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace dnsbenchmark {

struct Server {
    std::string name;
    std::string ip;
    std::uint16_t port = 53;
};

struct BenchmarkOptions {
    std::vector<std::string> server_inputs;
    int rounds = 3;
    int timeout_ms = 1500;
    bool export_csv = false;
    bool show_help = false;
    bool use_tui = false;
};

struct Stats {
    std::optional<double> min;
    std::optional<double> max;
    std::optional<double> avg;
    std::optional<double> median;
    std::optional<double> stddev;
    std::optional<double> jitter;
};

struct CsvRow {
    int round = 0;
    std::string server;
    std::string ip;
    std::uint16_t port = 53;
    std::string domain;
    std::optional<double> latency_ms;
};

struct SummaryRow {
    int rank = 0;
    Server server;
    std::string endpoint;
    Stats stats;
    double success_rate = 0.0;
    std::size_t success_count = 0;
    std::size_t total_queries = 0;
};

struct ServerMeasurements {
    Server server;
    std::vector<std::vector<double>> domain_latencies;
};

struct RoundServerResult {
    int round_number = 0;
    int total_rounds = 0;
    std::size_t server_index = 0;
    Server server;
    std::optional<double> average_ms;
    std::size_t failure_count = 0;
};

struct BenchmarkResult {
    std::vector<std::string> domains;
    std::vector<ServerMeasurements> measurements;
    std::vector<CsvRow> csv_rows;
    std::vector<SummaryRow> ranked_summary;
};

}  // namespace dnsbenchmark
