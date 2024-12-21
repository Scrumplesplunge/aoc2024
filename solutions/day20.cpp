#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>
#include <ranges>

namespace aoc2024 {
namespace {

struct Vec {
  friend bool operator==(const Vec&, const Vec&) = default;
  int x, y;
};

Vec operator*(Vec v, int s) { return Vec(v.x * s, v.y * s); }
Vec operator*(int s, Vec v) { return v * s; }
Vec operator+(Vec l, Vec r) { return Vec(l.x + r.x, l.y + r.y); }

constexpr Vec kOffsets[] = {Vec(0, -1), Vec(-1, 0), Vec(1, 0), Vec(0, 1)};

struct Input {
  struct Cell {
    std::uint16_t wall : 1;
    std::uint16_t time : 15;
  };

  Input() = default;

  // Non-copyable (self-referential).
  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;

  auto& operator[](this auto&& self, int x, int y) {
    assert(self.InBounds(Vec(x, y)));
    return self.grid[y][x];
  }

  auto& operator[](this auto&& self, Vec v) { return self[v.x, v.y]; }

  bool InBounds(Vec v) const {
    return 0 <= v.x && v.x < width && 0 <= v.y && v.y < height;
  }

  Task<void> Read(tcp::Socket& socket) {
    char input_buffer[20030];
    const std::string_view input(co_await socket.Read(input_buffer));
    if (input.empty() || input.back() != '\n') {
      throw std::runtime_error("bad input (truncated)");
    }

    // This must succeed: the input ends with '\n'.
    width = std::string_view(input).find('\n');
    if (input.size() % (width + 1) != 0) {
      throw std::runtime_error("grid is not rectangular");
    }
    height = input.size() / (width + 1);
    std::println("grid size: {}x{}", width, height);

    const auto start_index = input.find('S');
    if (start_index == input.npos) throw std::runtime_error("no start");
    start = Vec(start_index % (width + 1), start_index / (width + 1));

    const auto end_index = input.find('E');
    if (end_index == input.npos) throw std::runtime_error("no end");
    end = Vec(end_index % (width + 1), end_index / (width + 1));

    // Populate the walls.
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        grid[y][x] = Cell{.wall = input[y * (width + 1) + x] == '#', .time = 0};
      }
    }
    // Populate the times.
    Vec position = start;
    std::uint16_t time = 1;
    (*this)[start].time = time;
    while (position != end) {
      bool found = false;
      for (Vec offset : kOffsets) {
        const Vec neighbour = position + offset;
        if (!(*this)[neighbour].wall && (*this)[neighbour].time == 0) {
          found = true;
          position = neighbour;
          break;
        }
      }
      if (!found) throw std::runtime_error("dead end");
      (*this)[position].time = ++time;
    }
    std::println("default path takes time={}", time);
  }

  Cell grid[141][141];
  int width, height;
  Vec start, end;
};

// 6945 too high
//   (fixed bug where I was considering diagonal offsets)
// 4446 too high
//   (fixed bug with considering cheats that start in walls)
// 1106 too low
//   (fixed bug with only counting one cheat per start position)
// 1378 correct answer.
int Part1(const Input& input) {
  int count = 0;
  const int x_max = input.width - 1;
  const int y_max = input.height - 1;
  int saving_by_count[100] = {};
  for (int y = 1; y < y_max; y++) {
    for (int x = 1; x < x_max; x++) {
      const Vec position = Vec(x, y);
      if (input[position].wall) continue;
      for (Vec offset : kOffsets) {
        if (!input.InBounds(position + 2 * offset)) continue;
        if (!input[position + offset].wall) continue;
        if (input[position + 2 * offset].wall) continue;
        const int time_saved =
            input[position + 2 * offset].time - input[position].time - 2;
        if (time_saved >= 100) count++;
        if (time_saved > 0) {
          if (time_saved < 100) saving_by_count[time_saved]++;
        }
      }
    }
  }
  for (int i = 0; i < 100; i++) {
    if (saving_by_count[i] == 0) continue;
    std::println("{} cheats that save {} picoseconds", saving_by_count[i], i);
  }
  return count;
}

}  // namespace

Task<void> Day20(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const int part2 = 0;
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024