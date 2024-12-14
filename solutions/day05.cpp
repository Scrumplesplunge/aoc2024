#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

Task<void> Day05(tcp::Socket& socket) {
  char buffer[16000];
  std::string_view input(co_await socket.Read(buffer));

  // ordered[a][b] is true if `a|b` is a constraint.
  bool ordered[100][100] = {};
  // Parse the list of dependencies.
  while (!ScanPrefix(input, "\n")) {
    std::int8_t a, b;
    if (!ScanPrefix(input, "{}|{}\n", a, b)) {
      throw std::runtime_error("bad constraint");
    }
    ordered[a][b] = true;
  }

  // Process each list of pages.
  int part1 = 0, part2 = 0;
  while (!input.empty()) {
    // Parse the list.
    constexpr int kMaxValues = 30;
    int values[kMaxValues];
    if (!ScanPrefix(input, "{}", values[0])) {
      throw std::runtime_error("no values in line");
    }
    int num_values = 1;
    while (!ScanPrefix(input, "\n")) {
      if (num_values == kMaxValues) {
        throw std::runtime_error("too many values in line");
      }
      if (!ScanPrefix(input, ",{}", values[num_values++])) {
        std::println("stuff bork at {}", input.data() - buffer);
        std::println("remaining:\n{}", input.substr(0, 50));
        throw std::runtime_error("bad syntax in line");
      }
    }

    bool correct = true;
    for (int i = 1; i < num_values; i++) {
      // Loop invariant: values[0..i) are sorted according to the constraints.
      for (int j = 0; j < i; j++) {
        if (ordered[values[i]][values[j]]) {
          // values[j] needs to be after values[i] and is the leftmost element
          // of values[0..i) which has that property. Insert right before it.
          std::rotate(values + j, values + i, values + i + 1);
          correct = false;
          break;
        }
      }
    }
    if (correct) {
      part1 += values[num_values / 2];
    } else {
      part2 += values[num_values / 2];
    }
  }

  std::println("part1: {}\npart2: {}\n", part1, part2);

  {
    char result[32];
    const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
    co_await socket.Write(std::span<const char>(result, end - result));
  }
}

}  // namespace aoc2024
