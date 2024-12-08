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

struct Vec2 {
  friend bool operator==(const Vec2&, const Vec2&) = default;
  friend Vec2 operator-(Vec2 l, Vec2 r) { return Vec2(l.x - r.x, l.y - r.y); }
  friend Vec2 operator+(Vec2 l, Vec2 r) { return Vec2(l.x + r.x, l.y + r.y); }
  std::int8_t x, y;
};

struct Antenna {
  char frequency;
  Vec2 position;
};

constexpr int kGridSize = 50;

struct Input {
  static constexpr int kMaxAntennas = 200;

  Task<void> Read(tcp::Socket& socket) {
    char buffer[2560];
    std::string_view input(co_await socket.Read(buffer));
    assert(input.size() == (kGridSize + 1) * kGridSize);

    for (std::int8_t y = 0; y < kGridSize; y++) {
      const std::string_view line =
          input.substr(y * (kGridSize + 1), kGridSize + 1);
      if (line.back() != '\n') throw std::runtime_error("bad grid shape");
      for (std::int8_t x = 0; x < kGridSize; x++) {
        if (line[x] == '.') continue;
        if (!std::isalnum(line[x])) throw std::runtime_error("bad character");
        if (num_antennas == kMaxAntennas) {
          throw std::runtime_error("too many antennas");
        }
        Antenna& antenna = antennas[num_antennas++];
        antenna.frequency = line[x];
        antenna.position = Vec2(x, y);
      }
    }

    std::ranges::sort(std::span(antennas, num_antennas), std::less<>(),
                      &Antenna::frequency);
  }

  int num_antennas = 0;
  Antenna antennas[kMaxAntennas];
};

bool SameFrequency(const Antenna& a, const Antenna& b) {
  return a.frequency == b.frequency;
}

int CountTrue(bool (&grid)[kGridSize][kGridSize]) {
  int count = 0;
  for (auto& row : grid) {
    for (bool cell : row) {
      if (cell) count++;
    }
  }
  return count;
}

bool InBounds(Vec2 position) {
  return 0 <= position.x && position.x < kGridSize &&
         0 <= position.y && position.y < kGridSize;
}

int Part1(std::span<const Antenna> antennas) {
  bool antinodes[kGridSize][kGridSize] = {};
  for (const auto frequency_group :
       std::ranges::views::chunk_by(antennas, SameFrequency)) {
    const int n = frequency_group.size();
    for (int a = 0; a < n; a++) {
      for (int b = 0; b < a; b++) {
        const Vec2 a_position = frequency_group[a].position;
        const Vec2 b_position = frequency_group[b].position;
        const Vec2 delta = b_position - a_position;
        for (Vec2 antinode : {a_position - delta, b_position + delta}) {
          if (InBounds(antinode)) antinodes[antinode.y][antinode.x] = true;
        }
      }
    }
  }
  return CountTrue(antinodes);
}

int Part2(std::span<const Antenna> antennas) {
  bool antinodes[kGridSize][kGridSize] = {};
  for (const auto frequency_group :
       std::ranges::views::chunk_by(antennas, SameFrequency)) {
    const int n = frequency_group.size();
    for (int a = 0; a < n; a++) {
      for (int b = 0; b < a; b++) {
        const Vec2 a_position = frequency_group[a].position;
        const Vec2 b_position = frequency_group[b].position;
        const Vec2 delta = b_position - a_position;
        // The sequence of antinodes forms a line tracing all the way across the
        // grid. Extend to one end of that line.
        Vec2 position = a_position;
        while (InBounds(position - delta)) position = position - delta;
        // Mark every antinode along the line.
        while (InBounds(position)) {
          antinodes[position.y][position.x] = true;
          position = position + delta;
        }
      }
    }
  }
  return CountTrue(antinodes);
}

}  // namespace

Task<void> Day08(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);
  const std::span<Antenna> antennas(input.antennas, input.num_antennas);

  // Part 1.
  const int part1 = Part1(antennas);
  const int part2 = Part2(antennas);

  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
