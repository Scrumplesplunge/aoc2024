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

constexpr int kGridSize = 140;
static constexpr int kBufferSize = (kGridSize + 1) * kGridSize + 1;

struct UnionFind {
  std::uint16_t Find(std::uint16_t id) {
    std::uint16_t root = id;
    while (nodes[root].parent != root) root = nodes[root].parent;
    while (id != root) id = std::exchange(nodes[id].parent, root);
    return root;
  }

  std::uint16_t Merge(std::uint16_t a, std::uint16_t b) {
    a = Find(a);
    b = Find(b);
    if (a == b) return a;
    if (nodes[a].size < nodes[b].size) {
      nodes[a].parent = b;
      nodes[b].size += nodes[a].size;
      return b;
    } else {
      nodes[b].parent = a;
      nodes[a].size += nodes[b].size;
      return a;
    }
  }

  struct Node {
    std::uint16_t parent;
    std::uint16_t size, perimeter, corners;
  };
  Node nodes[kBufferSize];
};

}  // namespace

Task<void> Day12(tcp::Socket& socket) {
  char buffer[kBufferSize];
  const std::string_view grid(co_await socket.Read(buffer));
  if (grid.size() == kBufferSize) throw std::runtime_error("input too big");
  if (grid.empty() || grid.back() != '\n') {
    throw std::runtime_error("no newline");
  }
  const int width = grid.find('\n');
  if (grid.size() % (width + 1) != 0) {
    throw std::runtime_error("not rectangular");
  }
  const int height = grid.size() / (width + 1);

  // Part 1.
  UnionFind groups;
  for (int i = 0, n = grid.size(); i < n; i++) {
    groups.nodes[i] = {
        .parent = std::uint16_t(i), .size = 1, .perimeter = 0, .corners = 0};
  }
  // Trace the outline perimeter.
  for (int x = 0, bottom = (height - 1) * (width + 1); x < width; x++) {
    groups.nodes[x].perimeter++;
    groups.nodes[bottom + x].perimeter++;
  }
  for (int y = 0; y < height; y++) {
    groups.nodes[y * (width + 1)].perimeter++;
    groups.nodes[y * (width + 1) + width - 1].perimeter++;
  }
  // Process corners on the outer perimeter.
  groups.nodes[0].corners++;
  groups.nodes[width - 1].corners++;
  groups.nodes[(height - 1) * (width + 1)].corners++;
  groups.nodes[height * (width + 1) - 2].corners++;
  for (int offset : {0, (height - 1) * (width + 1)}) {
    for (int x = 1; x < width; x++) {
      const std::uint16_t a = offset + x - 1, b = a + 1;
      if (grid[a] != grid[b]) {
        groups.nodes[a].corners++;
        groups.nodes[b].corners++;
      }
    }
  }
  for (int y = 1; y < height; y++) {
    for (int offset : {0, width - 1}) {
      const std::uint16_t a = (y - 1) * (width + 1) + offset, b = a + width + 1;
      if (grid[a] != grid[b]) {
        groups.nodes[a].corners++;
        groups.nodes[b].corners++;
      }
    }
  }
  // Process corners which are not on the outer perimeter.
  for (int y = 1; y < height; y++) {
    const std::uint16_t previous_row = (y - 1) * (width + 1),
                        current_row = previous_row + width + 1;
    for (int x = 1; x < width; x++) {
      int cells[] = {previous_row + x - 1, previous_row + x,
                     current_row + x, current_row + x - 1};
      for (int r = 0; r < 4; r++) {
        auto [a, b, c, d] = cells;
        const bool inner_corner =
            grid[a] == grid[b] && grid[a] == grid[d] && grid[a] != grid[c];
        const bool outer_corner = grid[a] != grid[b] && grid[a] != grid[d];
        if (inner_corner || outer_corner) {
          groups.nodes[a].corners++;
        }
        std::rotate(cells, cells + 1, cells + 4);
      }
    }
  }
  // Merge all adjacent cells which are part of the same plot.
  auto test = [&](std::uint16_t a, std::uint16_t b) {
    if (grid[a] == grid[b]) {
      groups.Merge(a, b);
    } else {
      groups.nodes[a].perimeter++;
      groups.nodes[b].perimeter++;
    }
  };
  for (int x = 1; x < width; x++) test(x - 1, x);
  for (int y = 1; y < height; y++) {
    const int above = (y - 1) * (width + 1);
    const int current = y * (width + 1);
    for (int x = 0; x < width; x++) {
      test(above + x, current + x);
      if (x > 0) test(current + x - 1, current + x);
    }
  }
  // Propagate perimeter and corner counts to the root nodes.
  for (int i = 0, n = grid.size(); i < n; i++) {
    const std::uint16_t j = groups.Find(i);
    if (j == i) continue;
    groups.nodes[j].perimeter += groups.nodes[i].perimeter;
    groups.nodes[j].corners += groups.nodes[i].corners;
  }
  // Calculate the total cost.
  std::uint64_t part1 = 0, part2 = 0;
  for (int i = 0, n = grid.size(); i < n; i++) {
    if (i % (width + 1) == width) continue;
    if (groups.Find(i) == i) {
      const UnionFind::Node& node = groups.nodes[i];
      part1 += node.size * node.perimeter;
      part2 += node.size * node.corners;
    }
  }

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
