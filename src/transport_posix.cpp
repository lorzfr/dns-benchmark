#include "dnsbenchmark/transport.hpp"

#include "dnsbenchmark/dns_protocol.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace dnsbenchmark {
namespace {

double round_one_decimal(const double value) {
    return std::round(value * 10.0) / 10.0;
}

#if defined(_WIN32)
class WsaSession {
public:
    WsaSession() {
        WSADATA data{};
        const int result = ::WSAStartup(MAKEWORD(2, 2), &data);
        ok_ = (result == 0);
    }

    ~WsaSession() {
        if (ok_) {
            ::WSACleanup();
        }
    }

    [[nodiscard]] bool ok() const {
        return ok_;
    }

private:
    bool ok_{false};
};
#endif

}  // namespace

std::optional<double> PosixUdpTransport::query(
    const std::string& ip,
    const std::uint16_t port,
    const std::string& domain,
    const int timeout_ms) {
#if defined(__unix__) || defined(__APPLE__)
    const auto query_packet = build_dns_a_query(domain);

    const int socket_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        return std::nullopt;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
        ::close(socket_fd);
        return std::nullopt;
    }

    const auto start_time = std::chrono::steady_clock::now();
    const auto sent = ::sendto(
        socket_fd,
        query_packet.packet.data(),
        query_packet.packet.size(),
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address));

    if (sent < 0 || static_cast<std::size_t>(sent) != query_packet.packet.size()) {
        ::close(socket_fd);
        return std::nullopt;
    }

    pollfd poll_fd{};
    poll_fd.fd = socket_fd;
    poll_fd.events = POLLIN;

    const auto poll_result = ::poll(&poll_fd, 1, timeout_ms);
    if (poll_result <= 0 || (poll_fd.revents & POLLIN) == 0) {
        ::close(socket_fd);
        return std::nullopt;
    }

    std::array<std::uint8_t, 512> buffer{};
    sockaddr_in source{};
    socklen_t source_length = sizeof(source);

    const auto received = ::recvfrom(
        socket_fd,
        buffer.data(),
        buffer.size(),
        0,
        reinterpret_cast<sockaddr*>(&source),
        &source_length);

    const auto end_time = std::chrono::steady_clock::now();
    ::close(socket_fd);

    if (received < 0) {
        return std::nullopt;
    }

    const std::vector<std::uint8_t> response(buffer.begin(), buffer.begin() + received);
    if (!is_valid_dns_response(response, query_packet.tx_id)) {
        return std::nullopt;
    }

    const auto elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    return round_one_decimal(elapsed_ms);
#elif defined(_WIN32)
    const auto query_packet = build_dns_a_query(domain);

    WsaSession wsa;
    if (!wsa.ok()) {
        return std::nullopt;
    }

    const SOCKET socket_fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_fd == INVALID_SOCKET) {
        return std::nullopt;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);

    if (::inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) {
        ::closesocket(socket_fd);
        return std::nullopt;
    }

    const auto start_time = std::chrono::steady_clock::now();
    const int sent = ::sendto(
        socket_fd,
        reinterpret_cast<const char*>(query_packet.packet.data()),
        static_cast<int>(query_packet.packet.size()),
        0,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address));

    if (sent < 0 || static_cast<std::size_t>(sent) != query_packet.packet.size()) {
        ::closesocket(socket_fd);
        return std::nullopt;
    }

    fd_set read_set{};
    FD_ZERO(&read_set);
    FD_SET(socket_fd, &read_set);

    timeval timeout{};
    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    const int select_result = ::select(0, &read_set, nullptr, nullptr, &timeout);
    if (select_result <= 0 || !FD_ISSET(socket_fd, &read_set)) {
        ::closesocket(socket_fd);
        return std::nullopt;
    }

    std::array<std::uint8_t, 512> buffer{};
    sockaddr_in source{};
    int source_length = sizeof(source);

    const int received = ::recvfrom(
        socket_fd,
        reinterpret_cast<char*>(buffer.data()),
        static_cast<int>(buffer.size()),
        0,
        reinterpret_cast<sockaddr*>(&source),
        &source_length);

    const auto end_time = std::chrono::steady_clock::now();
    ::closesocket(socket_fd);

    if (received < 0) {
        return std::nullopt;
    }

    const std::vector<std::uint8_t> response(buffer.begin(), buffer.begin() + received);
    if (!is_valid_dns_response(response, query_packet.tx_id)) {
        return std::nullopt;
    }

    const auto elapsed_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();
    return round_one_decimal(elapsed_ms);
#else
    (void)ip;
    (void)port;
    (void)domain;
    (void)timeout_ms;
    throw std::runtime_error("Udp transport is unsupported on this target.");
#endif
}

}  // namespace dnsbenchmark
