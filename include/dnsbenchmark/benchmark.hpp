#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

#include "dnsbenchmark/transport.hpp"
#include "dnsbenchmark/types.hpp"

namespace dnsbenchmark {

using RoundCallback = std::function<void(const RoundServerResult&)>;

Stats calculate_stats(const std::vector<double>& values);
std::optional<double> calculate_domain_median(
    const ServerMeasurements& measurements,
    std::size_t domain_index);

std::vector<SummaryRow> build_summary(
    const BenchmarkResult& result,
    int rounds);

BenchmarkResult run_benchmark(
    const BenchmarkOptions& options,
    const std::vector<Server>& servers,
    const std::vector<std::string>& domains,
    UdpTransport& transport,
    const RoundCallback& on_round_result = {});

}  // namespace dnsbenchmark
