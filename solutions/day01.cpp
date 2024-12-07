#include "../common/coro.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char buffer[14001];
    std::span<const char> input = co_await socket.Read(buffer);
    assert(input.size() == 14000);

    for (int i = 0; i < 1000; i++) {
      // Here we are requiring exactly 14 bytes per line:
      //   * 5 bytes for the first number
      //   * 3 spaces
      //   * 5 bytes for the second number
      //   * A newline
      std::span<const char> line = input.subspan(14 * i, 14);
      assert(line[5] == ' ' && line[6] == ' ' && line[7] == ' ');
      assert(line[13] == '\n');

      a[i] = std::stoi(std::string(line.data(), 5));
      b[i] = std::stoi(std::string(line.data() + 8, 5));
    }
  }

  int a[1000];
  int b[1000];
};

Task<void> Day01(tcp::Socket& socket) {
  std::println("parsing input...");
  Input input;
  co_await input.Read(socket);

  std::println("sorting...");
  std::ranges::sort(input.a);
  std::ranges::sort(input.b);

  int delta = 0;
  for (int i = 0; i < 1000; i++) {
    delta += std::abs(input.a[i] - input.b[i]);
  }

  std::println("part 1: {}", delta);

  int score = 0;
  for (int i = 0; i < 1000; i++) {
    auto [first, last] = std::ranges::equal_range(input.b, input.a[i]);
    score += input.a[i] * (last - first);
  }

  std::println("part 2: {}", score);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", delta, score);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
