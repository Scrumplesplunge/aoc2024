#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

Task<void> Day05(tcp::Socket& socket) {
  char buffer[16000];
  const std::string_view input(co_await socket.Read(buffer));
  assert(input.size() < 16000);  // If we hit 16k then we may have truncated.

  // `dependencies[b]` is a list of values `a` such that `a|b` is a constraint.
  std::vector<int> dependencies[100];
  // Parse the list of dependencies.
  const char* i = input.data();
  const char* const end = i + input.size();
  while (*i != '\n') {
    assert(end - i >= 6);
    assert(std::isdigit(i[0]) && std::isdigit(i[1]) &&
           i[2] == '|' &&
           std::isdigit(i[3]) && std::isdigit(i[4]) &&
           i[5] == '\n');
    const int a = 10 * (i[0] - '0') + (i[1] - '0');
    const int b = 10 * (i[3] - '0') + (i[4] - '0');
    std::println("{}|{}", a, b);
    dependencies[b].push_back(a);
    i += 6;
  }
  assert(*i == '\n');
  i++;

  // Process each list of pages.
  int part1 = 0, part2 = 0;
  while (i != end) {
    // Parse the list.
    constexpr int kMaxValues = 30;
    int values[kMaxValues];
    int num_values = 0;
    while (true) {
      // Parse a value.
      assert(num_values < kMaxValues);
      assert(end - i >= 3);
      assert(std::isdigit(i[0]) && std::isdigit(i[1]));
      values[num_values++] = 10 * (i[0] - '0') + (i[1] - '0');
      const char lookahead = i[2];
      i += 3;
      if (lookahead == '\n') break;
      assert(lookahead == ',');
    }
    for (int i = 0; i < num_values; i++) {
      std::print(" {}", values[i]);
    }

    bool correct = true;
    for (int i = 1; i < num_values; i++) {
      // Loop invariant: values[0..i) are sorted according to the constraints.
      for (int j = 0; j < i; j++) {
        if (std::ranges::contains(dependencies[values[j]], values[i])) {
          // values[j] needs to be after values[i] and is the leftmost element
          // of values[0..i) which has that property. Insert right before it.
          std::rotate(values + j, values + i, values + i + 1);
          correct = false;
          break;
        }
      }
    }
    std::print(" ->");
    for (int i = 0; i < num_values; i++) {
      std::print(" {}", values[i]);
    }
    std::println("");
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
