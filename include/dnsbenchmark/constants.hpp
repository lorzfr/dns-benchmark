#pragma once

#include <string>
#include <vector>

#include "dnsbenchmark/types.hpp"

namespace dnsbenchmark {

const std::vector<Server>& built_in_servers();
const std::vector<std::string>& test_domains();

Server parse_server_entry(const std::string& raw);
std::vector<Server> resolve_servers(const std::vector<std::string>& inputs);

std::string benchmark_mode(bool using_custom_servers);
std::string endpoint_string(const Server& server);

}  // namespace dnsbenchmark
