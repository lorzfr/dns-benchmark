#include "test_common.hpp"

#include "dnsbenchmark/benchmark.hpp"
#include "dnsbenchmark/cli.hpp"
#include "dnsbenchmark/constants.hpp"
#include "dnsbenchmark/dns_protocol.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

dnsbenchmark::BenchmarkOptions parse_args(const std::vector<std::string>& args) {
    std::vector<std::string> storage;
    storage.push_back("dnsbenchmark");
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (auto& item : storage) {
        argv.push_back(item.data());
    }

    return dnsbenchmark::parse_arguments(static_cast<int>(argv.size()), argv.data());
}

void test_parse_server_entry_known_ip() {
    const auto server = dnsbenchmark::parse_server_entry("1.1.1.1");
    ASSERT_EQ(std::string("Cloudflare Primary"), server.name);
    ASSERT_EQ(std::string("1.1.1.1"), server.ip);
    ASSERT_EQ(static_cast<std::uint16_t>(53), server.port);
}

void test_parse_server_entry_custom_port() {
    const auto server = dnsbenchmark::parse_server_entry("192.168.178.201:5335");
    ASSERT_EQ(std::string("192.168.178.201:5335"), server.name);
    ASSERT_EQ(std::string("192.168.178.201"), server.ip);
    ASSERT_EQ(static_cast<std::uint16_t>(5335), server.port);
}

void test_parse_arguments_gnu_style() {
    const auto options = parse_args({
        "--servers", "1.1.1.1", "8.8.4.4",
        "--rounds", "5",
        "--timeout", "2000",
        "--export-csv",
    });

    ASSERT_EQ(2U, options.server_inputs.size());
    ASSERT_EQ(std::string("1.1.1.1"), options.server_inputs[0]);
    ASSERT_EQ(std::string("8.8.4.4"), options.server_inputs[1]);
    ASSERT_EQ(5, options.rounds);
    ASSERT_EQ(2000, options.timeout_ms);
    ASSERT_TRUE(options.export_csv);
}

void test_parse_arguments_single_server_alias() {
    const auto options = parse_args({
        "--server", "192.168.178.201",
        "--timeout-ms", "1200",
    });

    ASSERT_EQ(1U, options.server_inputs.size());
    ASSERT_EQ(std::string("192.168.178.201"), options.server_inputs[0]);
    ASSERT_EQ(1200, options.timeout_ms);
}

void test_parse_arguments_powershell_aliases() {
    const auto options = parse_args({
        "-Servers", "1.1.1.1", "8.8.8.8",
        "-Rounds", "4",
        "-TimeoutMs", "1750",
        "-ExportCsv",
    });

    ASSERT_EQ(2U, options.server_inputs.size());
    ASSERT_EQ(4, options.rounds);
    ASSERT_EQ(1750, options.timeout_ms);
    ASSERT_TRUE(options.export_csv);
}

void test_dns_response_validation() {
    const std::array<std::uint8_t, 2> tx_id = {0x12, 0x34};
    const auto packet = dnsbenchmark::build_dns_a_query("example.com", tx_id);

    ASSERT_EQ(tx_id[0], packet.tx_id[0]);
    ASSERT_EQ(tx_id[1], packet.tx_id[1]);
    ASSERT_TRUE(packet.packet.size() > 12U);

    const std::vector<std::uint8_t> valid = {0x12, 0x34, 0x81, 0x80};
    const std::vector<std::uint8_t> wrong_id = {0x12, 0x35, 0x81, 0x80};
    const std::vector<std::uint8_t> bad_rcode = {0x12, 0x34, 0x81, 0x84};

    ASSERT_TRUE(dnsbenchmark::is_valid_dns_response(valid, tx_id));
    ASSERT_FALSE(dnsbenchmark::is_valid_dns_response(wrong_id, tx_id));
    ASSERT_FALSE(dnsbenchmark::is_valid_dns_response(bad_rcode, tx_id));
}

void test_calculate_stats_even_and_odd() {
    {
        const auto stats = dnsbenchmark::calculate_stats({4.0, 2.0, 8.0});
        ASSERT_TRUE(stats.avg.has_value());
        ASSERT_TRUE(stats.median.has_value());
        ASSERT_NEAR(4.7, *stats.avg, 0.01);
        ASSERT_NEAR(4.0, *stats.median, 0.01);
        ASSERT_NEAR(2.0, *stats.min, 0.01);
        ASSERT_NEAR(8.0, *stats.max, 0.01);
        ASSERT_NEAR(3.0, *stats.jitter, 0.01);
    }

    {
        const auto stats = dnsbenchmark::calculate_stats({2.0, 4.0, 6.0, 8.0});
        ASSERT_TRUE(stats.median.has_value());
        ASSERT_NEAR(5.0, *stats.median, 0.01);
    }
}

void test_calculate_stats_empty() {
    const auto stats = dnsbenchmark::calculate_stats({});
    ASSERT_FALSE(stats.avg.has_value());
    ASSERT_FALSE(stats.median.has_value());
    ASSERT_FALSE(stats.jitter.has_value());
}

}  // namespace

int main() {
    try {
        run_test("parse_server_entry_known_ip", test_parse_server_entry_known_ip);
        run_test("parse_server_entry_custom_port", test_parse_server_entry_custom_port);
        run_test("parse_arguments_gnu_style", test_parse_arguments_gnu_style);
        run_test("parse_arguments_single_server_alias", test_parse_arguments_single_server_alias);
        run_test("parse_arguments_powershell_aliases", test_parse_arguments_powershell_aliases);
        run_test("dns_response_validation", test_dns_response_validation);
        run_test("calculate_stats_even_and_odd", test_calculate_stats_even_and_odd);
        run_test("calculate_stats_empty", test_calculate_stats_empty);

        std::cout << "All unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
