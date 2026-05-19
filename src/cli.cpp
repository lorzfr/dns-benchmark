#include "dnsbenchmark/cli.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace dnsbenchmark {
namespace {

bool starts_with(const std::string& value, const std::string& prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string trim_copy(const std::string& value) {
    const auto begin = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    });
    const auto end = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) {
        return std::isspace(ch) != 0;
    }).base();

    if (begin >= end) {
        return {};
    }

    return std::string(begin, end);
}

void append_server_values(const std::string& raw, std::vector<std::string>& output) {
    std::size_t start = 0;

    while (start <= raw.size()) {
        const auto comma = raw.find(',', start);
        const auto chunk = raw.substr(start, comma == std::string::npos ? std::string::npos : comma - start);
        const auto trimmed = trim_copy(chunk);
        if (!trimmed.empty()) {
            output.push_back(trimmed);
        }

        if (comma == std::string::npos) {
            break;
        }

        start = comma + 1;
    }
}

bool is_known_flag_token(const std::string& token) {
    return token == "--help" ||
           token == "-h" ||
           token == "--server" ||
           token == "--servers" ||
           token == "--rounds" ||
           token == "--timeout" ||
           token == "--timeout-ms" ||
           token == "--export-csv" ||
           token == "--exportcsv" ||
           token == "--export" ||
           token == "--tui" ||
           starts_with(token, "--server=") ||
           starts_with(token, "--servers=") ||
           starts_with(token, "--rounds=") ||
           starts_with(token, "--timeout=") ||
           starts_with(token, "--timeout-ms=");
}

int parse_positive_int(const std::string& value, const std::string& label) {
    try {
        const auto parsed = std::stoi(value);
        if (parsed <= 0) {
            throw std::runtime_error(label + " must be greater than zero.");
        }

        return parsed;
    } catch (const std::exception&) {
        throw std::runtime_error(label + " must be a positive integer.");
    }
}

std::string executable_name_only(const std::string& path) {
    const auto slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

}  // namespace

BenchmarkOptions parse_arguments(int argc, char* argv[]) {
    BenchmarkOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string token = argv[index];

        if (token == "--help" || token == "-h") {
            options.show_help = true;
            continue;
        }

        if (token == "--server") {
            if (index + 1 >= argc || is_known_flag_token(argv[index + 1])) {
                throw std::runtime_error("The --server flag requires an IP or IP:Port value.");
            }

            append_server_values(argv[++index], options.server_inputs);
            continue;
        }

        if (starts_with(token, "--server=")) {
            append_server_values(token.substr(std::string("--server=").size()), options.server_inputs);
            if (options.server_inputs.empty()) {
                throw std::runtime_error("The --server flag requires an IP or IP:Port value.");
            }
            continue;
        }

        if (token == "--servers") {
            const auto start_count = options.server_inputs.size();

            while (index + 1 < argc && !is_known_flag_token(argv[index + 1])) {
                append_server_values(argv[++index], options.server_inputs);
            }

            if (options.server_inputs.size() == start_count) {
                throw std::runtime_error("The --servers flag requires at least one IP or IP:Port value.");
            }

            continue;
        }

        if (starts_with(token, "--servers=")) {
            append_server_values(token.substr(std::string("--servers=").size()), options.server_inputs);
            if (options.server_inputs.empty()) {
                throw std::runtime_error("The --servers flag requires at least one IP or IP:Port value.");
            }
            continue;
        }

        if (token == "--rounds") {
            if (index + 1 >= argc) {
                throw std::runtime_error("The --rounds flag requires an integer value.");
            }
            options.rounds = parse_positive_int(argv[++index], "Rounds");
            continue;
        }

        if (starts_with(token, "--rounds=")) {
            options.rounds = parse_positive_int(token.substr(std::string("--rounds=").size()), "Rounds");
            continue;
        }

        if (token == "--timeout" || token == "--timeout-ms") {
            if (index + 1 >= argc) {
                throw std::runtime_error("The --timeout flag requires an integer value in milliseconds.");
            }
            options.timeout_ms = parse_positive_int(argv[++index], "TimeoutMs");
            continue;
        }

        if (starts_with(token, "--timeout=")) {
            options.timeout_ms = parse_positive_int(token.substr(std::string("--timeout=").size()), "TimeoutMs");
            continue;
        }

        if (starts_with(token, "--timeout-ms=")) {
            options.timeout_ms = parse_positive_int(token.substr(std::string("--timeout-ms=").size()), "TimeoutMs");
            continue;
        }

        if (token == "--export-csv" || token == "--exportcsv" || token == "--export") {
            options.export_csv = true;
            continue;
        }

        if (token == "--tui") {
            options.use_tui = true;
            continue;
        }

        throw std::runtime_error("Unrecognized argument '" + token + "'. Use --help for usage.");
    }

    return options;
}

std::string build_help_text(const std::string& executable_name) {
    std::ostringstream help;
    const auto exe = executable_name_only(executable_name);

    help
        << "DNS Benchmark\n"
        << "\n"
        << "Usage:\n"
        << "  " << exe << " [--server <server>] [--servers <server...>] [--rounds <int>] [--timeout <ms>] [--timeout-ms <ms>] [--export-csv] [--tui]\n"
        << "  " << exe << " [--servers <server...>] [--rounds <int>] [--timeout <ms>] [--export-csv] [--tui]\n"
        << "\n"
        << "Servers accept either IP or IP:Port.\n"
        << "\n"
        << "Examples:\n"
        << "  " << exe << "\n"
        << "  " << exe << " --server 192.168.178.201\n"
        << "  " << exe << " --servers 1.1.1.1 8.8.4.4 --rounds 5 --timeout 2000 --export-csv\n"
        << "  " << exe << " --tui\n"
        << "  " << exe << " -Servers 1.1.1.1 8.8.8.8 -Rounds 5 -TimeoutMs 2000 -ExportCsv\n";

    return help.str();
}

}  // namespace dnsbenchmark
