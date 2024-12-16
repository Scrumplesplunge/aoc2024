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
  struct Cell {
    std::uint32_t seen : 1;
    std::uint32_t cost : 31;
  };

  auto& operator[](this auto&& self, Vec position, Direction direction) {
    assert(position.x < 141 && position.y < 141);
    assert(position.x % 2 == 1 && position.y % 2 == 1);
    return self.cells[position.y / 2][position.x / 2][direction];
  }

  bool Insert(Vec position, Direction direction, int cost) {
    Cell& cell = (*this)[position, direction];
    if (cell.seen) return false;
    cell = Cell{.seen = 1, .cost = std::uint32_t(cost)};
    return true;
  }

  // `cells[y][x][d].seen` is set if we've already been at `(2x+1, 2y+1)` facing
  // direction `d`, and `cost` is the cost of getting there the first time.
  Cell cells[70][70][4] = {};
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

int Part2(Grid grid, const VisitedSet& visited, Vec start, Vec end,
          Direction end_direction) {
  struct Node {
    Vec position;
    Direction direction;
  };
  Node stack[1000];
  int stack_size = 0;
  std::uint8_t seen[141][141] = {};
  stack[stack_size++] = Node(end, end_direction);
  while (stack_size > 0) {
    const Node node = stack[--stack_size];
    const Vec p = node.position;
    // If `seen` is already set, we've counted tiles on this path already.
    if (seen[p.y][p.x] & (1 << node.direction)) continue;
    seen[p.y][p.x] |= 1 << node.direction;
    if (p == start) continue;
    if (stack_size > 996) throw std::runtime_error("stack overflow");
    // There should never be a wall behind us (the direction of a visited
    // location is the direction we came from).
    const Vec gap = p + Rotate(node.direction, 2);
    assert(grid[gap] != '#');
    seen[gap.y][gap.x] = true;
    const Vec position = Step(p, Rotate(node.direction, 2), 2);
    // Otherwise, check all possible directions for how we might have got here.
    const int cost = visited[p, node.direction].cost;
    for (int rotation = -1; rotation < 3; rotation++) {
      const Direction direction = Rotate(node.direction, rotation);
      const VisitedSet::Cell& cell = visited[position, direction];
      // If `cell.seen` is not set, the location is not part of any best path.
      if (!cell.seen) continue;
      // We can only have come from this location if the cost adds up.
      if (cell.cost + 1000 * std::abs(rotation) + 2 != cost) continue;
      static constexpr const char* kDir[] = {"up", "right", "down", "left"};
      std::println("{},{},{} is legit", position.x, position.y,
                   kDir[direction]);
      stack[stack_size++] = Node{position, direction};
    }
  }
  int total = 0;
  for (int y = 0; y < 141; y++) {
    for (int x = 0; x < 141; x++) {
      if (seen[y][x]) total++;
    }
  }
  for (int y = 0; y < grid.height; y++) {
    for (int x = 0; x < grid.width; x++) {
      std::print("{}", seen[y][x] ? 'O' : grid[x, y]);
    }
    std::println();
  }
  std::println("debug: 3,11,1 is {} with cost {}",
               visited[Vec(3, 11), kRight].seen ? "seen" : "not seen",
               visited[Vec(3, 11), kRight].cost);
  return total;
}

struct Result { int part1, part2; };

Result Solve(Input& input) {
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
    const bool is_new =
        visited.Insert(node.position, node.direction, node.cost);
    if (!is_new) continue;
    if (node.position == input.end) {
      const int part1 = node.cost;
      const int part2 =
          Part2(input.grid, visited, input.start, input.end, node.direction);
      return {part1, part2};
    }
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

  const auto [part1, part2] = Solve(input);
  std::println("part1: {}\npart2: {}\n", part1, part2);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
