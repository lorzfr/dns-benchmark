#pragma once

#include <string>

#include "dnsbenchmark/types.hpp"

namespace dnsbenchmark {

BenchmarkOptions parse_arguments(int argc, char* argv[]);
std::string build_help_text(const std::string& executable_name);

}  // namespace dnsbenchmark
