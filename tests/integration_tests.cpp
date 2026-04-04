#include "test_common.hpp"

#include "dnsbenchmark/benchmark.hpp"
#include "dnsbenchmark/output.hpp"
#include "dnsbenchmark/transport.hpp"
#include "dnsbenchmark/types.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

class FakeDnsServer {
public:
    struct Config {
        bool respond = true;
        int delay_ms = 0;
        std::uint8_t rcode = 0;
    };

    explicit FakeDnsServer(Config config)
        : config_(config) {
        socket_fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd_ < 0) {
            throw std::runtime_error("Could not create fake DNS socket.");
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(0);

        if (::bind(socket_fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
            ::close(socket_fd_);
            throw std::runtime_error("Could not bind fake DNS socket.");
        }

        sockaddr_in bound{};
        socklen_t bound_length = sizeof(bound);
        if (::getsockname(socket_fd_, reinterpret_cast<sockaddr*>(&bound), &bound_length) != 0) {
            ::close(socket_fd_);
            throw std::runtime_error("Could not query fake DNS port.");
        }

        port_ = ntohs(bound.sin_port);
        thread_ = std::thread([this]() { this->serve(); });
    }

    ~FakeDnsServer() {
        stop_.store(true);
        if (thread_.joinable()) {
            thread_.join();
        }
        if (socket_fd_ >= 0) {
            ::close(socket_fd_);
        }
    }

    std::uint16_t port() const {
        return port_;
    }

private:
    void serve() {
        while (!stop_.load()) {
            pollfd poll_fd{};
            poll_fd.fd = socket_fd_;
            poll_fd.events = POLLIN;

            const auto poll_result = ::poll(&poll_fd, 1, 50);
            if (poll_result <= 0 || (poll_fd.revents & POLLIN) == 0) {
                continue;
            }

            std::array<std::uint8_t, 512> buffer{};
            sockaddr_in source{};
            socklen_t source_length = sizeof(source);
            const auto received = ::recvfrom(
                socket_fd_,
                buffer.data(),
                buffer.size(),
                0,
                reinterpret_cast<sockaddr*>(&source),
                &source_length);

            if (received < 2) {
                continue;
            }

            if (!config_.respond) {
                continue;
            }

            if (config_.delay_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(config_.delay_ms));
            }

            const std::array<std::uint8_t, 4> response = {
                buffer[0],
                buffer[1],
                0x81,
                static_cast<std::uint8_t>(0x80 | (config_.rcode & 0x0F)),
            };

            ::sendto(
                socket_fd_,
                response.data(),
                response.size(),
                0,
                reinterpret_cast<const sockaddr*>(&source),
                source_length);
        }
    }

    Config config_;
    int socket_fd_ = -1;
    std::uint16_t port_ = 0;
    std::atomic<bool> stop_{false};
    std::thread thread_;
};

dnsbenchmark::Server make_server(const std::string& name, const std::uint16_t port) {
    return dnsbenchmark::Server{name, "127.0.0.1", port};
}

void test_valid_reply_path() {
    FakeDnsServer server({true, 10, 0});
    dnsbenchmark::PosixUdpTransport transport;

    const auto latency = transport.query("127.0.0.1", server.port(), "example.com", 500);
    ASSERT_TRUE(latency.has_value());
    ASSERT_TRUE(*latency >= 0.0);
}

void test_timeout_failure_path() {
    FakeDnsServer server({false, 0, 0});
    dnsbenchmark::PosixUdpTransport transport;

    const auto latency = transport.query("127.0.0.1", server.port(), "example.com", 50);
    ASSERT_FALSE(latency.has_value());
}

void test_custom_port_path() {
    FakeDnsServer server({true, 0, 0});
    dnsbenchmark::PosixUdpTransport transport;

    const auto latency = transport.query("127.0.0.1", server.port(), "example.com", 200);
    ASSERT_TRUE(latency.has_value());
}

void test_ranking_and_success_rate() {
    FakeDnsServer fast_server({true, 5, 0});
    FakeDnsServer slow_server({true, 25, 0});
    FakeDnsServer dead_server({false, 0, 0});

    dnsbenchmark::BenchmarkOptions options;
    options.rounds = 1;
    options.timeout_ms = 150;

    const std::vector<dnsbenchmark::Server> servers = {
        make_server("Fast DNS", fast_server.port()),
        make_server("Slow DNS", slow_server.port()),
        make_server("Dead DNS", dead_server.port()),
    };
    const std::vector<std::string> domains = {"alpha.test", "beta.test"};

    dnsbenchmark::PosixUdpTransport transport;
    const auto result = dnsbenchmark::run_benchmark(options, servers, domains, transport);

    ASSERT_EQ(3U, result.ranked_summary.size());
    ASSERT_EQ(std::string("Fast DNS"), result.ranked_summary[0].server.name);
    ASSERT_EQ(std::string("Slow DNS"), result.ranked_summary[1].server.name);
    ASSERT_EQ(std::string("Dead DNS"), result.ranked_summary[2].server.name);
    ASSERT_NEAR(100.0, result.ranked_summary[0].success_rate, 0.01);
    ASSERT_NEAR(100.0, result.ranked_summary[1].success_rate, 0.01);
    ASSERT_NEAR(0.0, result.ranked_summary[2].success_rate, 0.01);
}

void test_csv_generation() {
    FakeDnsServer server({true, 0, 0});

    dnsbenchmark::BenchmarkOptions options;
    options.rounds = 1;
    options.timeout_ms = 200;
    options.export_csv = true;

    const std::vector<dnsbenchmark::Server> servers = {
        make_server("CSV DNS", server.port()),
    };
    const std::vector<std::string> domains = {"csv.test"};

    dnsbenchmark::PosixUdpTransport transport;
    const auto result = dnsbenchmark::run_benchmark(options, servers, domains, transport);

    const auto path = std::filesystem::temp_directory_path() / "dnsbenchmark-integration.csv";
    dnsbenchmark::write_csv(path, result.csv_rows);

    std::ifstream in(path);
    ASSERT_TRUE(in.good());

    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    ASSERT_TRUE(content.find("Round,Server,IP,Port,Domain,LatencyMs") != std::string::npos);
    ASSERT_TRUE(content.find("CSV DNS") != std::string::npos);
    ASSERT_TRUE(content.find("127.0.0.1") != std::string::npos);
    ASSERT_TRUE(content.find("csv.test") != std::string::npos);

    std::filesystem::remove(path);
}

}  // namespace

int main() {
    try {
        run_test("valid_reply_path", test_valid_reply_path);
        run_test("timeout_failure_path", test_timeout_failure_path);
        run_test("custom_port_path", test_custom_port_path);
        run_test("ranking_and_success_rate", test_ranking_and_success_rate);
        run_test("csv_generation", test_csv_generation);

        std::cout << "All integration tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
