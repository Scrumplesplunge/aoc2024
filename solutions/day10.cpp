#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>
#include <ranges>

namespace aoc2024 {
namespace {

constexpr int kSize = 47;

struct Vec { std::int8_t x, y; };

struct Input {
  auto& operator[](this auto&& self, int x, int y) {
    assert(0 <= x && x <= self.width && 0 <= y && y <= self.height);
    return self.text[y * (self.width + 1) + x];
  }

  bool InBounds(int x, int y) const {
    return 0 <= x && x < width && 0 <= y && y < height;
  }

  bool InBounds(Vec v) const { return InBounds(v.x, v.y); }

  std::string_view text;
  int width, height;
};

static constexpr int kDeltas[][2] = {{1, 0}, {-1, 0}, {0, -1}, {0, 1}};

Task<Input> ReadInput(tcp::Socket& socket, std::span<char> buffer) {
  std::string_view input(co_await socket.Read(buffer));
  assert(input.size() <= (kSize + 1) * kSize && input.back() == '\n');
  const int width = input.find('\n');
  assert(width <= kSize);
  assert(input.size() % (width + 1) == 0);
  const int height = input.size() / (width + 1);
  assert(height <= kSize);
  co_return Input(input, width, height);
}

// Returns the total number of '9' cells reachable from `(x, y)` (a cell
// containing value `c`) which have not already been seen according to `seen`.
int Explore(const Input& input, int x, int y, char c,
            bool (&seen)[kSize][kSize]) {
  if (seen[y][x]) return 0;  // Avoid double-counting.
  seen[y][x] = true;
  if (c == '9') return 1;
  int total = 0;
  for (const auto [dx, dy] : kDeltas) {
    if (input.InBounds(x + dx, y + dy) && input[x + dx, y + dy] == c + 1) {
      total += Explore(input, x + dx, y + dy, c + 1, seen);
    }
  }
  return total;
}

int Part1(const Input& input) {
  int total = 0;
  for (int y = 0; y < input.height; y++) {
    for (int x = 0; x < input.width; x++) {
      if (input[x, y] != '0') continue;
      bool seen[kSize][kSize] = {};
      total += Explore(input, x, y, '0', seen);
    }
  }
  return total;
}

int Part2(const Input& input) {
  // `count[y][x]` is the number of paths from trailheads to `(x, y)`.
  std::uint16_t count[kSize][kSize] = {};
  // `nodes[last_step..next_node)` is the frontier of positions which are each
  // `step` units along a trail from a trailhead. The `nodes` array is filled
  // linearly without reuse and `next_node` is the index of the first unused
  // entry in the array.
  Vec nodes[kSize * kSize];
  int last_step = 0;
  int next_node = 0;
  // Add all trailheads to the `count` and `nodes` arrays.
  for (int y = 0; y < input.height; y++) {
    for (int x = 0; x < input.width; x++) {
      if (input[x, y] == '0') {
        count[y][x] = 1;
        nodes[next_node++] = Vec(x, y);
      }
    }
  }
  // One step at a time, move the frontier forward.
  for (int step = 1; step <= 9; step++) {
    const std::span<const Vec> previous =
        std::span(nodes).subspan(last_step, next_node - last_step);
    last_step = next_node;
    for (Vec node : previous) {
      for (const auto [dx, dy] : kDeltas) {
        const Vec n = Vec(node.x + dx, node.y + dy);
        if (input.InBounds(n) && input[n.x, n.y] == '0' + step) {
          if (count[n.y][n.x] == 0) nodes[next_node++] = n;
          count[n.y][n.x] += count[node.y][node.x];
        }
      }
    }
  }
  // Sum up the total number of paths to '9' cells.
  int total = 0;
  for (int i = last_step; i < next_node; i++) {
    const Vec n = nodes[i];
    assert((input[n.x, n.y] == '9'));
    total += count[n.y][n.x];
  }
  return total;
}

}  // namespace

Task<void> Day10(tcp::Socket& socket) {
  char buffer[2257];
  const Input input = co_await ReadInput(socket, buffer);

  const int part1 = Part1(input);
  const int part2 = Part2(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
