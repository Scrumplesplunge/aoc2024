#include "../common/coro.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {

Task<void> Day01(tcp::Socket& socket) {
  int a[1000];
  int b[1000];

  std::println("parsing input...");
  for (int i = 0; i < 1000; i++) {
    // Here we are requiring exactly 14 bytes per line:
    //   * 5 bytes for the first number
    //   * 3 spaces
    //   * 5 bytes for the second number
    //   * A newline
    char buffer[14];
    std::span<const char> line = co_await socket.Read(buffer);
    if (line.size() != 14) {
      co_await socket.Write("bad input");
      co_return;
    }
    assert(line[5] == ' ' && line[6] == ' ' && line[7] == ' ');
    assert(line[13] == '\n');

    a[i] = std::stoi(std::string(line.data(), 5));
    b[i] = std::stoi(std::string(line.data() + 8, 5));
  }

  std::println("sorting...");
  std::ranges::sort(a);
  std::ranges::sort(b);

  int delta = 0;
  for (int i = 0; i < 1000; i++) {
    delta += std::abs(a[i] - b[i]);
  }

  std::println("part 1: {}", delta);

  int score = 0;
  for (int i = 0; i < 1000; i++) {
    auto [first, last] = std::ranges::equal_range(b, a[i]);
    score += a[i] * (last - first);
  }

  std::println("part 2: {}", score);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", delta, score);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
