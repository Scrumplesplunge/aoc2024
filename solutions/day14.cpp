#include <algorithm>
#include <cctype>
#include <cstring>
#include <print>
#include <ranges>
#include <iostream>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

using std::literals::operator""sv;

struct Vec { int x, y; };
struct Robot { Vec p, v; };

constexpr Vec operator+(Vec l, Vec r) { return Vec(l.x + r.x, l.y + r.y); }
constexpr Vec operator*(int s, Vec v) { return Vec(s * v.x, s * v.y); }
constexpr Vec operator/(Vec v, int s) { return Vec(v.x / s, v.y / s); }

constexpr Vec operator%(Vec a, Vec b) {
  assert(b.x > 0 && b.y > 0);
  a.x %= b.x;
  a.y %= b.y;
  if (a.x < 0) a.x += b.x;
  if (a.y < 0) a.y += b.y;
  return a;
}

Task<std::span<Robot>> ReadInput(tcp::Socket& socket,
                                 std::span<Robot> robots) {
  const int max_robots = robots.size();
  int num_robots = 0;
  char buffer[10000];
  std::string_view input(co_await socket.Read(buffer));
  while (!input.empty()) {
    if (num_robots == max_robots) {
      throw std::runtime_error("too many robots");
    }
    Robot& robot = robots[num_robots++];
    if (!ScanPrefix(input, "p={},{} v={},{}\n", robot.p.x, robot.p.y, robot.v.x,
                    robot.v.y)) {
      throw std::runtime_error("bad robot description");
    }
  }
  co_return robots.subspan(0, num_robots);
}

constexpr Vec kGridSize = {101, 103};
constexpr Vec kMiddle = kGridSize / 2;

// 78001875 too low.
//   (fixed bug with negative velocity values).
// 231777252 too low.
//   (fixed copy-paste bug with the incorrect middle value for y).
// 236628054 correct.
std::int64_t Part1(std::span<const Robot> robots) {
  std::int64_t top_left = 0, top_right = 0, bottom_left = 0, bottom_right = 0;
  for (Robot robot : robots) {
    const Vec p = (robot.p + 100 * robot.v) % kGridSize;
    const bool left = p.x < kMiddle.x;
    const bool right = p.x > kMiddle.x;
    const bool top = p.y < kMiddle.y;
    const bool bottom = p.y > kMiddle.y;
    if (top && left) top_left++;
    if (top && right) top_right++;
    if (bottom && left) bottom_left++;
    if (bottom && right) bottom_right++;
  }
  std::println("top_left={}, top_right={}, bottom_left={}, bottom_right={}",
               top_left, top_right, bottom_left, bottom_right);
  return top_left * top_right * bottom_left * bottom_right;
}

std::int64_t Part2(std::span<Robot> robots) {
  int time = 0;
  while (true) {
    time++;
    for (Robot& robot : robots) {
      robot.p = (robot.p + robot.v) % kGridSize;
    }
    char wtf[kGridSize.y * (kGridSize.x + 1)];
    std::ranges::fill(wtf, '.');
    for (int y = 0; y < kGridSize.y; y++) {
      wtf[y * (kGridSize.x + 1) + kGridSize.x] = '\n';
    }
    for (const Robot robot : robots) {
      const int i = robot.p.y * (kGridSize.x + 1) + robot.p.x;
      if (wtf[i] == '.') {
        wtf[i] = '1';
      } else if ('1' <= wtf[i] && wtf[i] < '9') {
        wtf[i]++;
      } else {
        wtf[i] = '*';
      }
    }
    int horizontalness = 0;
    for (int y = 0; y < kGridSize.y; y++) {
      const int o = y * (kGridSize.x + 1);
      for (int x = 1; x < kGridSize.x; x++) {
        if (wtf[o + x] != '.' && wtf[o + x - 1] != '.') horizontalness++;
      }
    }
    if (horizontalness > 200) {
      std::print("\x1b[2J{}time={}\n\n", std::string_view(wtf, sizeof(wtf)),
                 time);
      std::cin.get();
    }
  }
}

}  // namespace

Task<void> Day14(tcp::Socket& socket) {
  Robot buffer[500];
  std::span<Robot> robots = co_await ReadInput(socket, buffer);

  const std::int64_t part1 = Part1(robots);
  const std::int64_t part2 = Part2(robots);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
