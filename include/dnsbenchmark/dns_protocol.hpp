#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace dnsbenchmark {

struct DnsQueryPacket {
    std::vector<std::uint8_t> packet;
    std::array<std::uint8_t, 2> tx_id{};
};

DnsQueryPacket build_dns_a_query(const std::string& domain);
DnsQueryPacket build_dns_a_query(
    const std::string& domain,
    const std::array<std::uint8_t, 2>& tx_id);

bool is_valid_dns_response(
    const std::vector<std::uint8_t>& response,
    const std::array<std::uint8_t, 2>& expected_tx_id);

}  // namespace dnsbenchmark
