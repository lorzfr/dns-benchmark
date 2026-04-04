#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace dnsbenchmark {

class UdpTransport {
public:
    virtual ~UdpTransport() = default;

    virtual std::optional<double> query(
        const std::string& ip,
        std::uint16_t port,
        const std::string& domain,
        int timeout_ms) = 0;
};

class PosixUdpTransport : public UdpTransport {
public:
    std::optional<double> query(
        const std::string& ip,
        std::uint16_t port,
        const std::string& domain,
        int timeout_ms) override;
};

}  // namespace dnsbenchmark
