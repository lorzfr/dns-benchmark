#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

#include "dnsbenchmark/types.hpp"

namespace dnsbenchmark {

bool stdout_supports_color();
std::filesystem::path make_default_csv_path(const std::filesystem::path& executable_path);
void write_csv(const std::filesystem::path& path, const std::vector<CsvRow>& rows);

class ConsoleRenderer {
public:
    explicit ConsoleRenderer(std::ostream& out, bool use_color);

    void print_banner(
        const std::string& mode,
        const std::vector<Server>& servers,
        const std::vector<std::string>& domains,
        const BenchmarkOptions& options) const;

    void print_round_result(const RoundServerResult& result) const;
    void print_summary(const BenchmarkResult& result) const;
    void print_per_domain_breakdown(const BenchmarkResult& result) const;
    void print_winner(const BenchmarkResult& result) const;
    void print_exported(const std::filesystem::path& csv_path) const;
    void print_finished() const;

private:
    std::ostream& out_;
    bool use_color_;
};

}  // namespace dnsbenchmark
