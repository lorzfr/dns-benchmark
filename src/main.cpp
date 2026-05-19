#include "dnsbenchmark/benchmark.hpp"
#include "dnsbenchmark/cli.hpp"
#include "dnsbenchmark/constants.hpp"
#include "dnsbenchmark/output.hpp"
#include "dnsbenchmark/transport.hpp"
#include "dnsbenchmark/tui.hpp"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[]) {
    try {
        auto options = dnsbenchmark::parse_arguments(argc, argv);
        if (options.show_help) {
            std::cout << dnsbenchmark::build_help_text(argc > 0 ? argv[0] : "dnsbenchmark");
            return 0;
        }

        if (options.use_tui) {
            if (dnsbenchmark::stdin_is_tty() && dnsbenchmark::launch_tui(options)) {
                return 0;
            }

            if (options.force_tui) {
                throw std::runtime_error("TUI mode requires an interactive terminal.");
            }
        }

        const auto servers = dnsbenchmark::resolve_servers(options.server_inputs);
        const auto& domains = dnsbenchmark::test_domains();

        dnsbenchmark::ConsoleRenderer renderer(std::cout, dnsbenchmark::stdout_supports_color());
        renderer.print_banner(
            dnsbenchmark::benchmark_mode(!options.server_inputs.empty()),
            servers,
            domains,
            options);

        dnsbenchmark::PosixUdpTransport transport;
        const auto result = dnsbenchmark::run_benchmark(
            options,
            servers,
            domains,
            transport,
            [&renderer](const dnsbenchmark::RoundServerResult& round_result) {
                renderer.print_round_result(round_result);
            });

        renderer.print_summary(result);
        renderer.print_per_domain_breakdown(result);
        renderer.print_winner(result);

        if (options.export_csv) {
            const auto csv_path = dnsbenchmark::make_default_csv_path(argc > 0 ? argv[0] : ".");
            dnsbenchmark::write_csv(csv_path, result.csv_rows);
            renderer.print_exported(csv_path);
        }

        renderer.print_finished();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
