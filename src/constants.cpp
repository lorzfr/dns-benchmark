#include "dnsbenchmark/constants.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace dnsbenchmark {
namespace {

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

const std::unordered_map<std::string, std::string>& known_names() {
    static const std::unordered_map<std::string, std::string> names = {
        {"8.8.8.8", "Google Primary"},
        {"8.8.4.4", "Google Secondary"},
        {"1.1.1.1", "Cloudflare Primary"},
        {"1.0.0.1", "Cloudflare Secondary"},
        {"9.9.9.9", "Quad9 (filtered)"},
        {"9.9.9.10", "Quad9 (unfiltered)"},
        {"208.67.222.222", "OpenDNS Primary"},
        {"208.67.220.220", "OpenDNS Secondary"},
        {"94.140.14.14", "AdGuard"},
        {"94.140.15.15", "AdGuard Secondary"},
    };

    return names;
}

bool all_digits(const std::string& value) {
    return !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
    });
}

void append_split_server_values(const std::string& raw, std::vector<std::string>& output) {
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

}  // namespace

const std::vector<Server>& built_in_servers() {
    static const std::vector<Server> servers = {
        {"Google Primary", "8.8.8.8", 53},
        {"Google Secondary", "8.8.4.4", 53},
        {"Cloudflare Primary", "1.1.1.1", 53},
        {"Cloudflare Secondary", "1.0.0.1", 53},
        {"Quad9 (filtered)", "9.9.9.9", 53},
        {"Quad9 (unfiltered)", "9.9.9.10", 53},
        {"OpenDNS Primary", "208.67.222.222", 53},
        {"OpenDNS Secondary", "208.67.220.220", 53},
        {"AdGuard", "94.140.14.14", 53},
    };

    return servers;
}

const std::vector<std::string>& test_domains() {
    static const std::vector<std::string> domains = {
        "google.com",
        "youtube.com",
        "facebook.com",
        "amazon.com",
        "github.com",
        "microsoft.com",
        "cloudflare.com",
        "reddit.com",
        "wikipedia.org",
        "stackoverflow.com",
    };

    return domains;
}

Server parse_server_entry(const std::string& raw) {
    const auto trimmed = trim_copy(raw);
    if (trimmed.empty()) {
        throw std::runtime_error("Server entries must not be empty.");
    }

    std::string ip = trimmed;
    std::uint16_t port = 53;

    const auto colon = trimmed.rfind(':');
    if (colon != std::string::npos) {
        const auto port_text = trimmed.substr(colon + 1);
        if (all_digits(port_text)) {
            const auto parsed_port = std::stoul(port_text);
            if (parsed_port == 0 || parsed_port > 65535) {
                throw std::runtime_error("Server port out of range in '" + trimmed + "'.");
            }

            ip = trim_copy(trimmed.substr(0, colon));
            port = static_cast<std::uint16_t>(parsed_port);
        }
    }

    if (ip.empty()) {
        throw std::runtime_error("Server IP must not be empty.");
    }

    std::string name;
    const auto name_it = known_names().find(ip);
    if (name_it != known_names().end()) {
        name = name_it->second;
        if (port != 53) {
            name += " :" + std::to_string(port);
        }
    } else {
        name = port != 53 ? ip + ":" + std::to_string(port) : ip;
    }

    return Server{name, ip, port};
}

std::vector<Server> resolve_servers(const std::vector<std::string>& inputs) {
    if (inputs.empty()) {
        return built_in_servers();
    }

    std::vector<std::string> flattened;
    for (const auto& input : inputs) {
        append_split_server_values(input, flattened);
    }

    if (flattened.empty()) {
        throw std::runtime_error("The --servers flag requires at least one IP or IP:Port value.");
    }

    std::vector<Server> servers;
    servers.reserve(flattened.size());
    for (const auto& raw : flattened) {
        servers.push_back(parse_server_entry(raw));
    }

    return servers;
}

std::string benchmark_mode(const bool using_custom_servers) {
    return using_custom_servers
        ? "Custom  (-Servers / --servers)"
        : "Built-in list (no -Servers flag given)";
}

std::string endpoint_string(const Server& server) {
    return server.ip + ":" + std::to_string(server.port);
}

}  // namespace dnsbenchmark
