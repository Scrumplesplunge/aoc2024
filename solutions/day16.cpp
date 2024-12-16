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
  std::uint8_t x, y;
};

enum Direction : std::uint8_t {
  // Direction values are sorted clockwise to simplify transformations.
  kUp,
  kRight,
  kDown,
  kLeft,
};

Vec Step(Vec v, Direction d, std::uint8_t distance) {
  switch (d) {
    case kUp:
      return Vec(v.x, v.y - distance);
    case kRight:
      return Vec(v.x + distance, v.y);
    case kDown:
      return Vec(v.x, v.y + distance);
    case kLeft:
      return Vec(v.x - distance, v.y);
  }
  std::abort();
}

Vec operator+(Vec v, Direction d) { return Step(v, d, 1); }

struct Grid {
  char& operator[](int x, int y) {
    assert(0 <= x && x < width && 0 <= y && y < height);
    return data[y * (width + 1) + x];
  }

  char& operator[](Vec v) { return (*this)[v.x, v.y]; }

  std::span<char> data;
  int width, height;
};

struct Input {
  Input() = default;

  // Non-copyable (self-referential).
  Input(const Input&) = delete;
  Input& operator=(const Input&) = delete;

  Task<void> Read(tcp::Socket& socket) {
    std::span<char> input = co_await socket.Read(input_buffer);
    if (input.empty() || input.back() != '\n') {
      throw std::runtime_error("bad input (truncated)");
    }

    // This must succeed: the input ends with '\n'.
    grid.width = std::string_view(input).find('\n');
    if (input.size() % (grid.width + 1) != 0) {
      throw std::runtime_error("grid is not rectangular");
    }
    grid.data = input;
    grid.height = input.size() / (grid.width + 1);

    const auto start_index = std::string_view(grid.data).find('S');
    if (start_index == std::string_view::npos) {
      throw std::runtime_error("no start");
    }
    start = Vec(start_index % (grid.width + 1), start_index / (grid.width + 1));

    const auto end_index = std::string_view(grid.data).find('E');
    if (end_index == std::string_view::npos) {
      throw std::runtime_error("no end");
    }
    end = Vec(end_index % (grid.width + 1), end_index / (grid.width + 1));
  }

  char input_buffer[24000];
  Grid grid;
  Vec start, end;
};

class Frontier {
 public:
  struct Node {
    // Cost to reach this position.
    int cost;
    // Actual cost plus heuristic estimate of the cost to reach the goal.
    int guess;
    Vec position;
    Direction direction;
  };

  bool Empty() const { return size_ == 0; }

  Node Pop() {
    assert(!Empty());
    std::ranges::pop_heap(data_, data_ + size_, std::greater<>(), &Node::guess);
    return data_[--size_];
  }

  void Push(Node node) {
    assert(size_ < kMaxSize);
    data_[size_++] = node;
    std::ranges::push_heap(data_, data_ + size_, std::greater<>(),
                           &Node::guess);
  }

 private:
  static constexpr int kMaxSize = 1000;
  Node data_[1000];
  int size_ = 0;
};

struct VisitedSet {
  bool Has(Vec position, Direction direction) const {
    assert(position.x < 141 && position.y < 141);
    return seen[position.y][position.x] & (1 << direction);
  }

  void Insert(Vec position, Direction direction) {
    assert(position.x < 141 && position.y < 141);
    seen[position.y][position.x] |= 1 << direction;
  }

  // `seen[y][x] & (1 << d)` is set if we've already been at `(x, y)` facing in
  // direction `d`.
  std::uint8_t seen[141][141] = {};
};

int GuessCost(Vec position, Vec end) {
  const int dx = std::abs(position.x - end.x),
            dy = std::abs(position.y - end.y);
  return dx + dy + (dx && dy ? 1000 : 0);
}

Direction Rotate(Direction direction, int num_clockwise_quarter_turns) {
  assert(-4 < num_clockwise_quarter_turns && num_clockwise_quarter_turns <= 4);
  return Direction((direction + num_clockwise_quarter_turns + 4) % 4);
}

int Part1(Input& input) {
  VisitedSet visited;
  Frontier frontier;
  frontier.Push({
      .cost = 0,
      .guess = GuessCost(input.start, input.end),
      .position = input.start,
      .direction = kRight,
  });
  while (!frontier.Empty()) {
    const Frontier::Node node = frontier.Pop();
    if (node.position == input.end) return node.cost;
    if (visited.Has(node.position, node.direction)) continue;
    visited.Insert(node.position, node.direction);
    for (int rotation = -1; rotation < 3; rotation++) {
      const Direction direction = Rotate(node.direction, rotation);
      if (input.grid[node.position + direction] == '#') continue;
      const Vec position = Step(node.position, direction, 2);
      const int cost = node.cost + 1000 * std::abs(rotation) + 2;
      frontier.Push({
          .cost = cost,
          .guess = cost + GuessCost(position, input.end),
          .position = position,
          .direction = direction,
      });
    }
  }
  throw std::runtime_error("end not found");
}

}  // namespace

Task<void> Day16(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  // 258760 too high
  //   fixed bug with forgetting to add the real cost to the heuristic guess
  // 130536 correct answer
  const int part1 = Part1(input);
  std::println("part1: {}\n", part1);

  char result[32];
  const char* end = std::format_to(result, "{}\n", part1);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
