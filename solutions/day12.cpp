#include <algorithm>
#include <cctype>
#include <cstring>
#include <print>
#include <ranges>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

static constexpr int kBufferSize = 19741;

struct Solver {
  explicit Solver(std::string_view grid, int width, int height)
      : grid(grid), width(width), height(height) {}

  struct Answer { int part1, part2; };

  Answer Run() {
    // Initialise the union-find nodes.
    for (int i = 0, n = grid.size(); i < n; i++) {
      nodes[i] = {
          .parent = std::uint16_t(i), .size = 1, .perimeter = 0, .corners = 0};
    }

    Part1();
    Part2();

    // Propagate perimeter and corner counts to the root nodes.
    for (int i = 0, n = grid.size(); i < n; i++) {
      const std::uint16_t j = Find(i);
      if (j == i) continue;
      nodes[j].perimeter += nodes[i].perimeter;
      nodes[j].corners += nodes[i].corners;
    }

    // Calculate the total cost.
    int part1 = 0, part2 = 0;
    for (int i = 0, n = grid.size(); i < n; i++) {
      if (i % (width + 1) == width) continue;
      if (Find(i) == i) {
        const Node& node = nodes[i];
        part1 += node.size * node.perimeter;
        part2 += node.size * node.corners;
      }
    }
    return {part1, part2};
  }

  void Part1() {
    // Check all edges between nodes to see if they need to be merged, or if
    // a perimeter edge needs to be counted.
    auto test = [&](std::uint16_t a, std::uint16_t b) {
      if (grid[a] == grid[b]) {
        Merge(a, b);
      } else {
        nodes[a].perimeter++;
        nodes[b].perimeter++;
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

    // The outermost edge is not between two nodes, so that has to be counted
    // separately.
    for (int x = 0, bottom = (height - 1) * (width + 1); x < width; x++) {
      nodes[x].perimeter++;
      nodes[bottom + x].perimeter++;
    }
    for (int y = 0; y < height; y++) {
      nodes[y * (width + 1)].perimeter++;
      nodes[y * (width + 1) + width - 1].perimeter++;
    }
  }

  // Annotate each cell with the number of exposed corners it has.
  void Part2() {
    // Process corners on the outer perimeter.
    nodes[0].corners++;
    nodes[width - 1].corners++;
    nodes[(height - 1) * (width + 1)].corners++;
    nodes[height * (width + 1) - 2].corners++;
    for (int offset : {0, (height - 1) * (width + 1)}) {
      for (int x = 1; x < width; x++) {
        const std::uint16_t a = offset + x - 1, b = a + 1;
        if (grid[a] != grid[b]) {
          nodes[a].corners++;
          nodes[b].corners++;
        }
      }
    }
    for (int y = 1; y < height; y++) {
      for (int offset : {0, width - 1}) {
        const std::uint16_t a = (y - 1) * (width + 1) + offset, b = a + width + 1;
        if (grid[a] != grid[b]) {
          nodes[a].corners++;
          nodes[b].corners++;
        }
      }
    }
    // Process corners which are not on the outer perimeter.
    for (int y = 1; y < height; y++) {
      const std::uint16_t previous_row = (y - 1) * (width + 1),
                          current_row = previous_row + width + 1;
      for (int x = 1; x < width; x++) {
        int cells[] = {previous_row + x - 1, previous_row + x, current_row + x,
                       current_row + x - 1};
        for (int r = 0; r < 4; r++) {
          auto [a, b, c, d] = cells;
          const bool inner_corner =
              grid[a] == grid[b] && grid[a] == grid[d] && grid[a] != grid[c];
          const bool outer_corner = grid[a] != grid[b] && grid[a] != grid[d];
          if (inner_corner || outer_corner) {
            nodes[a].corners++;
          }
          std::rotate(cells, cells + 1, cells + 4);
        }
      }
    }
  }

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
    // `parent` and `size` form a union-find data structure across nodes.
    // `parent` is the index of the parent node. A root node is its own parent.
    // A root node has an accurate `size` representing the number of elements in
    // that set. The `size` value is insignificant for non-root nodes.
    std::uint16_t parent, size;
    // `perimeter` is the total number of exposed edges. `corners` is the total
    // number of exposed corners. For non-root nodes, these totals are for the
    // single cell. For root nodes, the value is set to the combined total by
    // `PropagateToRoots()`.
    std::uint16_t perimeter, corners;
  };

  const std::string_view grid;
  const int width, height;
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

  Solver solver(grid, width, height);
  const auto [part1, part2] = solver.Run();
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
