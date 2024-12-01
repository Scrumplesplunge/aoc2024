#include "solve.hpp"

#include <print>

namespace aoc2024 {

#define DAYS(F)  \
    F(Day01, 1) F(Day02, 2) F(Day03, 3) F(Day04, 4) F(Day05, 5) F(Day06, 6)  \
    F(Day07, 7) F(Day08, 8) F(Day09, 9) F(Day10, 10) F(Day11, 11)  \
    F(Day12, 12) F(Day13, 13) F(Day14, 14) F(Day15, 15) F(Day16, 16)  \
    F(Day17, 17) F(Day18, 18) F(Day19, 19) F(Day20, 20) F(Day21, 21)  \
    F(Day22, 22) F(Day23, 23) F(Day24, 24) F(Day25, 25)

#define STUB(day, id)                                 \
  [[gnu::weak]] Task<void> day(tcp::Socket& socket) { \
    std::println(#day " is not solved.");             \
    co_await socket.Write(#day " is not solved.\n");  \
  }
DAYS(STUB)
#undef STUB

Task<void> Solve(int day, tcp::Socket& socket) {
  assert(1 <= day && day <= 25);
  switch (day) {
#define CASE(day, id) case id: return day(socket);
    DAYS(CASE)
#undef CASE
  }
  std::abort();
}

}  // namespace aoc2024
