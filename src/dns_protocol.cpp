#include "dnsbenchmark/dns_protocol.hpp"

#include <array>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace dnsbenchmark {
namespace {

std::array<std::uint8_t, 2> random_tx_id() {
    static thread_local std::mt19937 generator(std::random_device{}());
    static thread_local std::uniform_int_distribution<int> distribution(0, 255);

    return {
        static_cast<std::uint8_t>(distribution(generator)),
        static_cast<std::uint8_t>(distribution(generator)),
    };
}

}  // namespace

DnsQueryPacket build_dns_a_query(const std::string& domain) {
    return build_dns_a_query(domain, random_tx_id());
}

DnsQueryPacket build_dns_a_query(
    const std::string& domain,
    const std::array<std::uint8_t, 2>& tx_id) {
    if (domain.empty()) {
        throw std::runtime_error("Domain must not be empty.");
    }

    std::vector<std::uint8_t> packet;
    packet.reserve(64);

    packet.push_back(tx_id[0]);
    packet.push_back(tx_id[1]);
    packet.push_back(0x01);
    packet.push_back(0x00);
    packet.insert(packet.end(), {0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00});

    std::size_t start = 0;
    while (start <= domain.size()) {
        const auto dot = domain.find('.', start);
        const auto label = domain.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (label.empty() || label.size() > 63) {
            throw std::runtime_error("Invalid DNS label in domain '" + domain + "'.");
        }

        packet.push_back(static_cast<std::uint8_t>(label.size()));
        packet.insert(packet.end(), label.begin(), label.end());

        if (dot == std::string::npos) {
            break;
        }

        start = dot + 1;
    }

    packet.push_back(0x00);
    packet.insert(packet.end(), {0x00, 0x01, 0x00, 0x01});

    return DnsQueryPacket{packet, tx_id};
}

bool is_valid_dns_response(
    const std::vector<std::uint8_t>& response,
    const std::array<std::uint8_t, 2>& expected_tx_id) {
    if (response.size() < 4) {
        return false;
    }

    if (response[0] != expected_tx_id[0] || response[1] != expected_tx_id[1]) {
        return false;
    }

    const auto rcode = static_cast<std::uint8_t>(response[3] & 0x0F);
    return rcode <= 3;
}

}  // namespace dnsbenchmark
