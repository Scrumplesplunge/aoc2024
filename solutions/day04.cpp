#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

Task<void> Day04(tcp::Socket& socket) {
  char buffer[20000];
  const std::string_view input(co_await socket.Read(buffer));
  assert(input.size() < 20000);  // If we hit 20k then we may have truncated.

  constexpr int kSize = 140;
  if (input.size() != kSize * (kSize + 1)) {
    co_await socket.Write("bad input size\n");
    co_return;
  }

  const auto get = [&](int x, int y) -> char {
    return input[y * (kSize + 1) + x];
  };

  int part1 = 0;
  // Consider every possible position for an 'X'.
  for (int y = 0; y < kSize; y++) {
    for (int x = 0; x < kSize; x++) {
      // Look for "XMAS" starting at this position and going in any direction.
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          if (dx == 0 && dy == 0) continue;
          const int lx = x + 3 * dx, ly = y + 3 * dy;
          if (!(0 <= lx && lx < kSize && 0 <= ly && ly < kSize)) continue;
          bool match = true;
          for (int i = 0; i < 4; i++) {
            if (get(x + i * dx, y + i * dy) != "XMAS"[i]) {
              match = false;
              break;
            }
          }
          if (match) part1++;
        }
      }
    }
  }

  int part2 = 0;
  // Consider every possible position for the central 'A'.
  for (int y = 1; y < kSize - 1; y++) {
    for (int x = 1; x < kSize - 1; x++) {
      if (get(x, y) != 'A') continue;
      // Look at the four positions diagonally adjacent to the centre. These
      // must be one of the four combinations below to give "MAS" twice.
      char corners[5] = {get(x - 1, y - 1), get(x + 1, y - 1),
                         get(x + 1, y + 1), get(x - 1, y + 1), 0};
      const std::string_view c = corners;
      if (c == "MMSS" || c == "MSSM" || c == "SSMM" || c == "SMMS") part2++;
    }
  }
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
