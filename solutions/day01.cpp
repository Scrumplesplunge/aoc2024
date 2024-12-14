#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char buffer[14001];
    std::string_view input(co_await socket.Read(buffer));

    for (int i = 0; i < 1000; i++) {
      if (!ScanPrefix(input, "{}   {}\n", a[i], b[i])) {
        throw std::runtime_error("bad input");
      }
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
