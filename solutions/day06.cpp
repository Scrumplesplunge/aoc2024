
#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "tcp.hpp"

#include <cctype>
#include <algorithm>
#include <cstring>
#include <print>

namespace aoc2024 {
namespace {

enum Direction {
  kUp,
  kRight,
  kDown,
  kLeft,
};

struct Vec2 { int x, y; };

Vec2 Step(Vec2 start, Direction direction) {
  switch (direction) {
    case kUp:
      return Vec2{start.x, start.y - 1};
    case kRight:
      return Vec2{start.x + 1, start.y};
    case kDown:
      return Vec2{start.x, start.y + 1};
    case kLeft:
      return Vec2{start.x - 1, start.y};
  }
  std::abort();
}

Direction Rotate(Direction d) { return Direction((d + 1) % 4); }

struct Grid {
  static constexpr int kSize = 130;

  static bool InBounds(Vec2 v) {
    return 0 <= v.x && v.x < kSize && 0 <= v.y && v.y < kSize;
  }

  char& operator[](Vec2 v) const {
    assert(InBounds(v));
    return data[v.y * (kSize + 1) + v.x];
  }

  static constexpr Direction start_direction = kUp;
  Vec2 start_position;
  std::span<char> data;
};

struct VisitedSet {
  bool contains(Vec2 position) const {
    return data[position.y][position.x];
  }

  bool contains(Vec2 position, Direction direction) const {
    return data[position.y][position.x] & (1 << direction);
  }

  void insert(Vec2 position, Direction direction) {
    data[position.y][position.x] |= 1 << direction;
  }

  std::uint8_t data[Grid::kSize][Grid::kSize] = {};
};

Grid Parse(std::span<char> input) {
  // The input should be a square grid plus newlines for each line.
  assert(input.size() == (Grid::kSize + 1) * Grid::kSize);

  // Find the start position.
  const auto guard = std::ranges::find(input, '^');
  assert(guard != input.end());
  const int index = guard - input.begin();
  const Vec2 start_position =
      Vec2(index % (Grid::kSize + 1), index / (Grid::kSize + 1));

  return Grid{.start_position = start_position, .data = input};
}

int Part1(const Grid& grid) {
  VisitedSet visited;
  Vec2 position = grid.start_position;
  Direction direction = grid.start_direction;
  visited.insert(position, kUp);
  int num_visited = 1;
  while (true) {
    const Vec2 next = Step(position, direction);
    if (!Grid::InBounds(next)) break;
    if (grid[next] == '#') {
      // Rotate 90 degrees.
      direction = Rotate(direction);
    } else {
      // Move forwards.
      position = next;
    }
    if (!visited.contains(position)) num_visited++;
    visited.insert(position, direction);
  }
  return num_visited;
}

// Returns true if the guard eventually loops from the given configuration.
bool Loops(VisitedSet visited, Vec2 position, Direction direction,
           const Grid& grid) {
  while (true) {
    const Vec2 next = Step(position, direction);
    if (!Grid::InBounds(next)) return false;
    if (grid[next] == '#') {
      // Rotate 90 degrees.
      direction = Rotate(direction);
      if (visited.contains(position, direction)) return true;
      visited.insert(position, direction);
    } else {
      // Move forwards.
      position = next;
    }
  }
}

int Part2(const Grid& grid) {
  VisitedSet visited;
  Vec2 position = grid.start_position;
  Direction direction = grid.start_direction;
  visited.insert(position, direction);
  int obstacle_positions = 0;
  while (true) {
    const Vec2 next = Step(position, direction);
    if (!Grid::InBounds(next)) break;
    if (grid[next] == '#') {
      // Rotate 90 degrees.
      direction = Rotate(direction);
    } else {
      if (!visited.contains(next)) {
        // Temporarily insert an obstacle to see if it causes a loop.
        grid[next] = '#';
        if (Loops(visited, position, direction, grid)) obstacle_positions++;
        grid[next] = ' ';
      }
      // Move forwards.
      position = next;
    }
    visited.insert(position, direction);
  }
  return obstacle_positions;
}

}  // namespace

Task<void> Day06(tcp::Socket& socket) {
  char buffer[18000];
  const std::span<char> input = co_await socket.Read(buffer);

  const Grid grid = Parse(input);
  const int part1 = Part1(grid);
  const int part2 = Part2(grid);

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
