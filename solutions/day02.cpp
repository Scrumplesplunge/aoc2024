#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {

bool IsSafe(std::span<const std::int8_t> values) {
  assert(values.size() >= 2);
  const int a = values[0];
  const int b = values[1];
  const int delta = std::abs(b - a);
  if (delta < 1 || 3 < delta) return false;  // Initial values too far apart.
  const bool ascending = a < b;
  int previous = b;
  for (int i = 2, n = values.size(); i < n; i++) {
    if ((previous < values[i]) != ascending) return false;  // Direction change.
    const int delta = std::abs(values[i] - previous);
    if (delta < 1 || 3 < delta) return false;  // Values too far apart.
    previous = values[i];
  }
  return true;
}

bool IsMostlySafe(std::span<const std::int8_t> values) {
  assert(values.size() <= 8);
  std::int8_t buffer[8];
  std::memcpy(buffer, values.data() + 1, values.size() - 1);
  const std::span<std::int8_t> most_values(buffer, values.size() - 1);
  if (IsSafe(most_values)) return true;
  // This forward pass will overwrite to produce all sequences excluding one
  // element: 1 2 3 4; [0] 2 3 4; 0 [1] 2 4; 0 1 [2] 3.
  // The last iteration will write `buffer[n - 1]` which is one past the end of
  // `most_values` but within bounds for `buffer`.
  for (int i = 0, n = values.size(); i < n; i++) {
    buffer[i] = values[i];
    if (IsSafe(most_values)) return true;
  }
  return false;
}

Task<void> Day02(tcp::Socket& socket) {
  char buffer[20000];
  std::string_view input(co_await socket.Read(buffer));

  int num_safe = 0;
  int num_mostly_safe = 0;
  while (!input.empty()) {
    // Parse the values.
    std::int8_t buffer[8];
    if (!ScanPrefix(input, "{}", buffer[0])) {
      throw std::runtime_error("no values in line");
    }
    int num_values = 1;
    while (!ScanPrefix(input, "\n")) {
      if (num_values == 8) throw std::runtime_error("too many values in line");
      if (!ScanPrefix(input, " {}", buffer[num_values++])) {
        throw std::runtime_error("bad syntax in line");
      }
    }
    const std::span<const std::int8_t> values(buffer, num_values);
    num_safe += IsSafe(values);
    num_mostly_safe += IsMostlySafe(values);
  }

  std::println("part1: {}\npart2: {}", num_safe, num_mostly_safe);

  char result[32];
  const char* end =
      std::format_to(result, "{}\n{}\n", num_safe, num_mostly_safe);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
