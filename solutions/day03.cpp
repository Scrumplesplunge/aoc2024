#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

Task<void> Day03(tcp::Socket& socket) {
  char buffer[20000];
  std::string_view input(co_await socket.Read(buffer));
  assert(input.size() < 20000);  // If we hit 20k then we may have truncated.

  bool enable = true;
  int part1 = 0;
  int part2 = 0;
  while (true) {
    // Skip to the next possible starting position.
    const auto i = input.find_first_of("dm");
    if (i == input.npos) break;  // No more matches.
    input.remove_prefix(i);
    if (ScanPrefix(input, "do()")) {
      enable = true;
    } else if (ScanPrefix(input, "don't()")) {
      enable = false;
    } else if (int a, b; ScanPrefix(input, "mul({},{})", a, b)) {
      part1 += a * b;
      if (enable) part2 += a * b;
    } else {
      // No match at the starting position. Search again starting after it.
      input.remove_prefix(1);
    }
  }

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
