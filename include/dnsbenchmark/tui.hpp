#pragma once

#include "dnsbenchmark/types.hpp"

namespace dnsbenchmark {

bool stdin_is_tty();
bool stdout_is_tty();
bool launch_tui(BenchmarkOptions& options);

}  // namespace dnsbenchmark
