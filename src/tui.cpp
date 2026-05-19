#include "dnsbenchmark/tui.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <io.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#endif

namespace dnsbenchmark {
namespace {

enum class Color { Default, Muted, Accent, Success, Warning, Error, Title };

std::string color_code(Color color) {
    switch (color) {
        case Color::Muted: return "\033[90m";
        case Color::Accent: return "\033[36m";
        case Color::Success: return "\033[32m";
        case Color::Warning: return "\033[33m";
        case Color::Error: return "\033[31m";
        case Color::Title: return "\033[97m";
        case Color::Default:
        default: return "\033[0m";
    }
}

std::string paint(const std::string& text, Color color, bool on) {
    if (!on || color == Color::Default) return text;
    return color_code(color) + text + color_code(Color::Default);
}

struct TerminalSize { int cols = 80; };

TerminalSize terminal_size() {
    TerminalSize s;
#if !defined(_WIN32)
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        s.cols = ws.ws_col;
    }
#endif
    s.cols = std::max(60, std::min(140, s.cols));
    return s;
}

std::string trim(std::string v) {
    while (!v.empty() && std::isspace(static_cast<unsigned char>(v.back())) != 0) v.pop_back();
    auto it = std::find_if_not(v.begin(), v.end(), [](unsigned char c){ return std::isspace(c) != 0; });
    v.erase(v.begin(), it);
    return v;
}

std::vector<std::string> parse_servers(const std::string& raw) {
    std::vector<std::string> out;
    std::size_t start = 0;
    while (start <= raw.size()) {
        auto comma = raw.find(',', start);
        auto part = trim(raw.substr(start, comma == std::string::npos ? std::string::npos : comma - start));
        if (!part.empty()) out.push_back(part);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return out;
}

void print_header(bool use_color) {
    const auto width = terminal_size().cols;
    const std::string line(width, '=');
    std::cout << "\n" << paint(line, Color::Accent, use_color) << "\n";
    std::cout << paint(" DNS Benchmark Setup (TUI)", Color::Title, use_color) << "\n";
    std::cout << paint(" Clean, adaptive terminal UI for benchmark configuration", Color::Muted, use_color) << "\n";
    std::cout << paint(line, Color::Accent, use_color) << "\n\n";
}

}  // namespace

bool stdin_is_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

bool stdout_is_tty() {
#if defined(_WIN32)
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool launch_tui(BenchmarkOptions& options) {
    const bool use_color = stdout_is_tty();
    print_header(use_color);

    std::string line;
    std::cout << paint("Servers (comma-separated IP or IP:Port, blank = built-in list): ", Color::Accent, use_color);
    if (!std::getline(std::cin, line)) return false;
    options.server_inputs = parse_servers(line);

    while (true) {
        std::cout << paint("Rounds [default 3]: ", Color::Accent, use_color);
        if (!std::getline(std::cin, line)) return false;
        line = trim(line);
        if (line.empty()) { options.rounds = 3; break; }
        try { options.rounds = std::stoi(line); } catch (...) { options.rounds = 0; }
        if (options.rounds > 0) break;
        std::cout << paint("Please enter a positive integer.\n", Color::Error, use_color);
    }

    while (true) {
        std::cout << paint("Timeout in ms [default 1500]: ", Color::Accent, use_color);
        if (!std::getline(std::cin, line)) return false;
        line = trim(line);
        if (line.empty()) { options.timeout_ms = 1500; break; }
        try { options.timeout_ms = std::stoi(line); } catch (...) { options.timeout_ms = 0; }
        if (options.timeout_ms > 0) break;
        std::cout << paint("Please enter a positive integer.\n", Color::Error, use_color);
    }

    std::cout << paint("Export CSV? [y/N]: ", Color::Accent, use_color);
    if (!std::getline(std::cin, line)) return false;
    line = trim(line);
    options.export_csv = !line.empty() && (line[0] == 'y' || line[0] == 'Y');

    std::cout << "\n" << paint("Configuration complete. Starting benchmark...\n", Color::Success, use_color);
    return true;
}

}  // namespace dnsbenchmark
