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
    std::uint16_t size, perimeter;
  };
  Node nodes[kBufferSize];
};

std::uint64_t Part1(std::string_view grid, int width, int height) {
  UnionFind groups;
  for (int i = 0, n = grid.size(); i < n; i++) {
    groups.nodes[i] = {.parent = std::uint16_t(i), .size = 1, .perimeter = 0};
  }
  // Trace the outline.
  for (int x = 0, bottom = (height - 1) * (width + 1); x < width; x++) {
    groups.nodes[x].perimeter++;
    groups.nodes[bottom + x].perimeter++;
  }
  for (int y = 0; y < height; y++) {
    groups.nodes[y * (width + 1)].perimeter++;
    groups.nodes[y * (width + 1) + width - 1].perimeter++;
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
  // Propagate perimeter counts to the root nodes.
  for (int i = 0, n = grid.size(); i < n; i++) {
    const std::uint16_t j = groups.Find(i);
    if (j == i) continue;
    groups.nodes[j].perimeter += groups.nodes[i].perimeter;
  }
  // Calculate the total cost.
  std::uint64_t cost = 0;
  for (int i = 0, n = grid.size(); i < n; i++) {
    if (groups.Find(i) == i) {
      cost += groups.nodes[i].size * groups.nodes[i].perimeter;
    }
  }
  return cost;
}

}  // namespace

Task<void> Day12(tcp::Socket& socket) {
  char buffer[kBufferSize];
  const std::string_view input(co_await socket.Read(buffer));
  if (input.size() == kBufferSize) throw std::runtime_error("input too big");
  if (input.empty() || input.back() != '\n') {
    throw std::runtime_error("no newline");
  }
  const int width = input.find('\n');
  if (input.size() % (width + 1) != 0) {
    throw std::runtime_error("not rectangular");
  }
  const int height = input.size() / (width + 1);

  // Part 1.
  const std::uint64_t part1 = Part1(input, width, height);
  const std::uint64_t part2 = 0;

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
