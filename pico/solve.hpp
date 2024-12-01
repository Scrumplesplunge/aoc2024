#ifndef AOC2024_SOLVE_HPP_
#define AOC2024_SOLVE_HPP_

#include "../common/coro.hpp"
#include "tcp.hpp"

namespace aoc2024 {

Task<void> Solve(int day, tcp::Socket& socket);

}  // namespace aoc2024

#endif  // AOC2024_SOLVE_HPP_
