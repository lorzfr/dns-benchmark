#include "dnsbenchmark/output.hpp"

#include "dnsbenchmark/benchmark.hpp"
#include "dnsbenchmark/constants.hpp"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace dnsbenchmark {
namespace {

enum class Color {
    Default,
    White,
    Cyan,
    Yellow,
    Green,
    Magenta,
    Red,
    DarkRed,
    DarkYellow,
    DarkGray,
};

std::string color_code(const Color color) {
    switch (color) {
        case Color::White:
            return "\033[37m";
        case Color::Cyan:
            return "\033[36m";
        case Color::Yellow:
            return "\033[33m";
        case Color::Green:
            return "\033[32m";
        case Color::Magenta:
            return "\033[35m";
        case Color::Red:
            return "\033[31m";
        case Color::DarkRed:
            return "\033[31;2m";
        case Color::DarkYellow:
            return "\033[33;2m";
        case Color::DarkGray:
            return "\033[90m";
        case Color::Default:
        default:
            return "\033[0m";
    }
}

std::string apply_color(const std::string& text, const Color color, const bool use_color) {
    if (!use_color || color == Color::Default) {
        return text;
    }

    return color_code(color) + text + color_code(Color::Default);
}

double round_one_decimal(const double value) {
    return std::round(value * 10.0) / 10.0;
}

std::string format_compact_number(const std::optional<double>& value) {
    if (!value.has_value()) {
        return "FAIL";
    }

    std::ostringstream out;
    out << std::fixed << std::setprecision(1) << *value;
    auto text = out.str();
    if (text.size() > 2 && text.substr(text.size() - 2) == ".0") {
        text.erase(text.size() - 2);
    }
    return text;
}

std::string format_compact_number(const double value) {
    return format_compact_number(std::optional<double>(round_one_decimal(value)));
}

std::string format_ms(const std::optional<double>& value) {
    if (!value.has_value()) {
        return "   FAIL ";
    }

    std::ostringstream out;
    out << std::setw(7) << std::fixed << std::setprecision(1) << *value << " ms";
    return out.str();
}

std::string format_cell(const std::optional<double>& value, const std::string& unit) {
    if (!value.has_value()) {
        return "FAIL";
    }

    return format_compact_number(value) + " " + unit;
}

std::string format_percent(const double value) {
    return format_compact_number(value) + "%";
}

Color latency_color(const std::optional<double>& value) {
    if (!value.has_value()) {
        return Color::Red;
    }

    if (*value < 10.0) {
        return Color::Magenta;
    }
    if (*value < 30.0) {
        return Color::Green;
    }
    if (*value < 80.0) {
        return Color::Yellow;
    }
    if (*value < 150.0) {
        return Color::DarkYellow;
    }
    return Color::Red;
}

std::string pad_left(const std::string& text, const int width) {
    std::ostringstream out;
    out << std::setw(width) << text;
    return out.str();
}

std::string pad_right(const std::string& text, const int width) {
    std::ostringstream out;
    out << std::left << std::setw(width) << text << std::right;
    return out.str();
}

std::string current_timestamp(const char* format) {
    const auto now = std::time(nullptr);
    std::tm local_tm{};

#if defined(_WIN32)
    localtime_s(&local_tm, &now);
#else
    localtime_r(&now, &local_tm);
#endif

    char buffer[64];
    std::strftime(buffer, sizeof(buffer), format, &local_tm);
    return buffer;
}

std::string short_server_name(const std::string& name) {
    auto short_name = std::regex_replace(name, std::regex(" Primary"), "");
    short_name = std::regex_replace(short_name, std::regex(" Secondary"), " 2nd");
    short_name = std::regex_replace(short_name, std::regex("\\(.*\\)"), "");

    while (!short_name.empty() && std::isspace(static_cast<unsigned char>(short_name.back())) != 0) {
        short_name.pop_back();
    }

    return short_name;
}

}  // namespace

bool stdout_supports_color() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

std::filesystem::path make_default_csv_path(const std::filesystem::path& executable_path) {
    const auto base_dir = std::filesystem::absolute(executable_path).parent_path();
    return base_dir / ("DNS-Benchmark-" + current_timestamp("%Y%m%d-%H%M%S") + ".csv");
}

void write_csv(const std::filesystem::path& path, const std::vector<CsvRow>& rows) {
    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Could not open CSV output path: " + path.string());
    }

    out << "Round,Server,IP,Port,Domain,LatencyMs\n";
    for (const auto& row : rows) {
        out << row.round << ','
            << row.server << ','
            << row.ip << ','
            << row.port << ','
            << row.domain << ','
            << (row.latency_ms.has_value() ? format_compact_number(row.latency_ms) : "FAIL")
            << '\n';
    }
}

ConsoleRenderer::ConsoleRenderer(std::ostream& out, const bool use_color)
    : out_(out), use_color_(use_color) {}

void ConsoleRenderer::print_banner(
    const std::string& mode,
    const std::vector<Server>& servers,
    const std::vector<std::string>& domains,
    const BenchmarkOptions& options) const {
    const std::string line(72, '=');

    out_ << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << apply_color("  DNS RESOLVER BENCHMARK  --  Native Linux CLI", Color::Cyan, use_color_) << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << '\n'
         << apply_color("  Mode     : " + mode, Color::White, use_color_) << '\n'
         << apply_color("  Servers  : " + std::to_string(servers.size()), Color::White, use_color_) << '\n'
         << apply_color("  Domains  : " + std::to_string(domains.size()), Color::White, use_color_) << '\n'
         << apply_color("  Rounds   : " + std::to_string(options.rounds), Color::White, use_color_) << '\n'
         << apply_color("  Timeout  : " + std::to_string(options.timeout_ms) + " ms", Color::White, use_color_) << '\n'
         << apply_color("  Started  : " + current_timestamp("%Y-%m-%d %H:%M:%S"), Color::White, use_color_) << '\n'
         << '\n'
         << apply_color("  Servers to test:", Color::White, use_color_) << '\n';

    for (const auto& server : servers) {
        const auto tag = server.port != 53 ? " (port " + std::to_string(server.port) + ")" : "";
        out_ << apply_color(
            "    " + pad_right(server.name, 28) + " " + server.ip + tag,
            Color::Cyan,
            use_color_) << '\n';
    }
}

void ConsoleRenderer::print_round_result(const RoundServerResult& result) const {
    if (result.server_index == 0) {
        out_ << '\n'
             << apply_color(
                    "  -- Round " + std::to_string(result.round_number) + " / " + std::to_string(result.total_rounds),
                    Color::Yellow,
                    use_color_)
             << '\n';
    }

    std::ostringstream line;
    line << "    "
         << pad_right(result.server.name, 28)
         << " ("
         << result.server.ip
         << ":"
         << pad_right(std::to_string(result.server.port), 5)
         << ")  "
         << format_ms(result.average_ms);

    out_ << apply_color(line.str(), latency_color(result.average_ms), use_color_);

    if (result.failure_count > 0) {
        out_ << apply_color("  (" + std::to_string(result.failure_count) + " fail)", Color::DarkRed, use_color_);
    }

    out_ << '\n';
}

void ConsoleRenderer::print_summary(const BenchmarkResult& result) const {
    const std::string line(72, '=');
    const std::string divider(108, '-');

    out_ << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << apply_color("  RESULTS SUMMARY  (all rounds combined)", Color::Cyan, use_color_) << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << '\n';

    const auto header =
        pad_right("Rank", 4) + " " +
        pad_right("Server", 28) + " " +
        pad_right("Endpoint", 22) + " " +
        pad_left("Avg", 9) + " " +
        pad_left("Median", 9) + " " +
        pad_left("Min", 7) + " " +
        pad_left("Max", 7) + " " +
        pad_left("StdDev", 8) + " " +
        pad_left("Jitter", 8) + " " +
        pad_left("Success%", 8);

    out_ << apply_color(header, Color::White, use_color_) << '\n'
         << apply_color(divider, Color::DarkGray, use_color_) << '\n';

    for (const auto& row : result.ranked_summary) {
        const auto text =
            pad_right("#" + std::to_string(row.rank), 4) + " " +
            pad_right(row.server.name, 28) + " " +
            pad_right(row.endpoint, 22) + " " +
            pad_left(format_cell(row.stats.avg, "ms"), 9) + " " +
            pad_left(format_cell(row.stats.median, "ms"), 9) + " " +
            pad_left(format_cell(row.stats.min, "ms"), 7) + " " +
            pad_left(format_cell(row.stats.max, "ms"), 7) + " " +
            pad_left(format_cell(row.stats.stddev, "ms"), 8) + " " +
            pad_left(format_cell(row.stats.jitter, "ms"), 8) + " " +
            pad_left(format_percent(row.success_rate), 8);

        out_ << apply_color(text, latency_color(row.stats.avg), use_color_) << '\n';
    }

    out_ << apply_color(divider, Color::DarkGray, use_color_) << '\n'
         << '\n'
         << apply_color("  Latency legend:", Color::White, use_color_) << '\n'
         << apply_color("    < 10 ms   Outstanding", Color::Magenta, use_color_) << '\n'
         << apply_color("    < 30 ms   Excellent", Color::Green, use_color_) << '\n'
         << apply_color("    30-79 ms  Good", Color::Yellow, use_color_) << '\n'
         << apply_color("    80-149 ms Fair", Color::DarkYellow, use_color_) << '\n'
         << apply_color("    >= 150 ms Poor", Color::Red, use_color_) << '\n';
}

void ConsoleRenderer::print_per_domain_breakdown(const BenchmarkResult& result) const {
    const std::string line(72, '=');
    constexpr int domain_width = 22;
    constexpr int cell_width = 12;

    out_ << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << apply_color("  PER-DOMAIN BREAKDOWN  (median ms -- green = fastest)", Color::Cyan, use_color_) << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << '\n';

    std::string header = pad_right("Domain", domain_width);
    for (const auto& measurement : result.measurements) {
        auto short_name = short_server_name(measurement.server.name);
        if (static_cast<int>(short_name.size()) > cell_width - 1) {
            short_name = short_name.substr(0, cell_width - 1);
        }
        header += pad_left(short_name, cell_width);
    }

    out_ << apply_color(header, Color::White, use_color_) << '\n'
         << apply_color(
                std::string(domain_width + static_cast<int>(result.measurements.size()) * cell_width, '-'),
                Color::DarkGray,
                use_color_)
         << '\n';

    for (std::size_t domain_index = 0; domain_index < result.domains.size(); ++domain_index) {
        std::optional<double> best_median;

        for (const auto& measurement : result.measurements) {
            const auto median = calculate_domain_median(measurement, domain_index);
            if (median.has_value() && (!best_median.has_value() || *median < *best_median)) {
                best_median = median;
            }
        }

        out_ << apply_color(pad_right(result.domains[domain_index], domain_width), Color::White, use_color_);

        for (const auto& measurement : result.measurements) {
            const auto median = calculate_domain_median(measurement, domain_index);
            const auto text = pad_left(
                median.has_value() ? format_compact_number(median) + " ms" : std::string("FAIL"),
                cell_width);
            const auto color = median.has_value() && best_median.has_value() && *median == *best_median
                ? Color::Green
                : latency_color(median);

            out_ << apply_color(text, color, use_color_);
        }

        out_ << '\n';
    }
}

void ConsoleRenderer::print_winner(const BenchmarkResult& result) const {
    const std::string line(72, '=');
    if (result.ranked_summary.empty()) {
        return;
    }

    const auto& winner = result.ranked_summary.front();
    const auto winner_color = latency_color(winner.stats.median);

    out_ << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << apply_color("  WINNER", Color::Cyan, use_color_) << '\n'
         << apply_color(line, Color::Cyan, use_color_) << '\n'
         << '\n'
         << apply_color("  ## " + winner.server.name + "  (" + winner.endpoint + ")", winner_color, use_color_) << '\n'
         << apply_color(
                "     Median " + format_compact_number(winner.stats.median) +
                    " ms  |  Avg " + format_compact_number(winner.stats.avg) +
                    " ms  |  Success " + format_percent(winner.success_rate),
                winner_color,
                use_color_)
         << '\n'
         << '\n';
}

void ConsoleRenderer::print_exported(const std::filesystem::path& csv_path) const {
    out_ << apply_color("  Exported: " + csv_path.string(), Color::Cyan, use_color_) << '\n'
         << '\n';
}

void ConsoleRenderer::print_finished() const {
    out_ << apply_color("  Finished: " + current_timestamp("%Y-%m-%d %H:%M:%S"), Color::DarkGray, use_color_) << '\n'
         << '\n';
}

}  // namespace dnsbenchmark
