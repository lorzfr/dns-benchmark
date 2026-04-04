#include "dnsbenchmark/benchmark.hpp"

#include "dnsbenchmark/constants.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace dnsbenchmark {
namespace {

double round_one_decimal(const double value) {
    return std::round(value * 10.0) / 10.0;
}

double sort_key_for(const SummaryRow& row) {
    return row.stats.median.value_or(99999.0);
}

}  // namespace

Stats calculate_stats(const std::vector<double>& values) {
    if (values.empty()) {
        return {};
    }

    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());

    const auto count = sorted.size();
    const auto average = std::accumulate(sorted.begin(), sorted.end(), 0.0) / static_cast<double>(count);

    double median = 0.0;
    if (count % 2 == 0) {
        median = (sorted[(count / 2) - 1] + sorted[count / 2]) / 2.0;
    } else {
        median = sorted[count / 2];
    }

    double variance_sum = 0.0;
    for (const auto value : sorted) {
        variance_sum += std::pow(value - average, 2.0);
    }

    double jitter = 0.0;
    if (count > 1) {
        double diff_sum = 0.0;
        for (std::size_t index = 1; index < count; ++index) {
            diff_sum += std::abs(sorted[index] - sorted[index - 1]);
        }
        jitter = diff_sum / static_cast<double>(count - 1);
    }

    Stats stats;
    stats.min = round_one_decimal(sorted.front());
    stats.max = round_one_decimal(sorted.back());
    stats.avg = round_one_decimal(average);
    stats.median = round_one_decimal(median);
    stats.stddev = round_one_decimal(std::sqrt(variance_sum / static_cast<double>(count)));
    stats.jitter = round_one_decimal(jitter);
    return stats;
}

std::optional<double> calculate_domain_median(
    const ServerMeasurements& measurements,
    const std::size_t domain_index) {
    if (domain_index >= measurements.domain_latencies.size()) {
        return std::nullopt;
    }

    return calculate_stats(measurements.domain_latencies[domain_index]).median;
}

std::vector<SummaryRow> build_summary(
    const BenchmarkResult& result,
    const int rounds) {
    std::vector<SummaryRow> rows;
    rows.reserve(result.measurements.size());

    const auto total_queries_per_server =
        static_cast<std::size_t>(result.domains.size()) * static_cast<std::size_t>(rounds);

    for (const auto& measurement : result.measurements) {
        std::vector<double> all_values;
        std::size_t success_count = 0;

        for (const auto& domain_values : measurement.domain_latencies) {
            success_count += domain_values.size();
            all_values.insert(all_values.end(), domain_values.begin(), domain_values.end());
        }

        SummaryRow row;
        row.server = measurement.server;
        row.endpoint = endpoint_string(measurement.server);
        row.stats = calculate_stats(all_values);
        row.success_count = success_count;
        row.total_queries = total_queries_per_server;
        row.success_rate = total_queries_per_server == 0
            ? 0.0
            : round_one_decimal(
                  (static_cast<double>(success_count) / static_cast<double>(total_queries_per_server)) * 100.0);

        rows.push_back(row);
    }

    std::stable_sort(rows.begin(), rows.end(), [](const SummaryRow& left, const SummaryRow& right) {
        const auto left_key = sort_key_for(left);
        const auto right_key = sort_key_for(right);
        if (left_key != right_key) {
            return left_key < right_key;
        }

        return left.endpoint < right.endpoint;
    });

    for (std::size_t index = 0; index < rows.size(); ++index) {
        rows[index].rank = static_cast<int>(index + 1);
    }

    return rows;
}

BenchmarkResult run_benchmark(
    const BenchmarkOptions& options,
    const std::vector<Server>& servers,
    const std::vector<std::string>& domains,
    UdpTransport& transport,
    const RoundCallback& on_round_result) {
    BenchmarkResult result;
    result.domains = domains;
    result.measurements.reserve(servers.size());

    for (const auto& server : servers) {
        ServerMeasurements measurements;
        measurements.server = server;
        measurements.domain_latencies.resize(domains.size());
        result.measurements.push_back(std::move(measurements));
    }

    for (int round = 1; round <= options.rounds; ++round) {
        for (std::size_t server_index = 0; server_index < servers.size(); ++server_index) {
            const auto& server = servers[server_index];
            double round_total = 0.0;
            std::size_t success_count = 0;
            std::size_t failure_count = 0;

            for (std::size_t domain_index = 0; domain_index < domains.size(); ++domain_index) {
                const auto& domain = domains[domain_index];
                const auto latency = transport.query(server.ip, server.port, domain, options.timeout_ms);

                if (latency.has_value()) {
                    result.measurements[server_index].domain_latencies[domain_index].push_back(*latency);
                    round_total += *latency;
                    ++success_count;
                } else {
                    ++failure_count;
                }

                if (options.export_csv) {
                    result.csv_rows.push_back(CsvRow{
                        round,
                        server.name,
                        server.ip,
                        server.port,
                        domain,
                        latency,
                    });
                }
            }

            if (on_round_result) {
                RoundServerResult round_result;
                round_result.round_number = round;
                round_result.total_rounds = options.rounds;
                round_result.server_index = server_index;
                round_result.server = server;
                round_result.average_ms = success_count == 0
                    ? std::nullopt
                    : std::make_optional(round_one_decimal(round_total / static_cast<double>(success_count)));
                round_result.failure_count = failure_count;
                on_round_result(round_result);
            }
        }
    }

    result.ranked_summary = build_summary(result, options.rounds);
    return result;
}

}  // namespace dnsbenchmark
