#include <algorithm>
#include <print>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

struct Vec {
  friend bool operator==(const Vec&, const Vec&) = default;
  std::int8_t x, y;
};

Vec operator+(Vec l, Vec r) { return Vec(l.x + r.x, l.y + r.y); }

constexpr Vec kEnd = Vec(70, 70);

bool InBounds(Vec position) {
  return 0 <= position.x && position.x <= 70 &&
         0 <= position.y && position.y <= 70;
}

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    char buffer[20000];
    std::string_view input(co_await socket.Read(buffer));
    std::uint16_t time = 1;
    while (!input.empty()) {
      std::uint8_t x, y;
      if (!ScanPrefix(input, "{},{}\n", x, y)) {
        throw std::runtime_error("bad input");
      }
      cells[y][x] = time++;
    }
  }

  std::uint16_t operator[](int x, int y) const { return (*this)[Vec(x, y)]; }

  std::uint16_t operator[](Vec position) const {
    assert(InBounds(position));
    return cells[position.y][position.x];
  }

  std::uint16_t cells[71][71] = {};
};

class VisitedSet {
 public:
  bool Insert(Vec position) {
    if (Has(position)) return false;
    data_[position.y][position.x / 32] |= 1 << (position.x % 32);
    assert(Has(position));
    return true;
  }

  bool Has(Vec position) const {
    assert(InBounds(position));
    return data_[position.y][position.x / 32] & (1 << (position.x % 32));
  }

 private:
  std::uint32_t data_[71][3] = {};
};

class Frontier {
 public:
  struct Node {
    // Cost to reach this position.
    std::uint16_t cost;
    // Actual cost plus heuristic estimate of the cost to reach the goal.
    std::uint16_t guess;
    Vec position;
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
  static constexpr int kMaxSize = 10000;
  Node data_[kMaxSize];
  int size_ = 0;
};

std::uint16_t GuessCost(Vec position) {
  return std::abs(kEnd.x - position.x) + std::abs(kEnd.y - position.y);
}

int Part1(const Input& input) {
  VisitedSet visited;
  Frontier frontier;
  frontier.Push({
      .cost = 0,
      .guess = GuessCost(Vec(0, 0)),
      .position = Vec(0, 0),
  });
  while (!frontier.Empty()) {
    const Frontier::Node node = frontier.Pop();
    if (!visited.Insert(node.position)) continue;
    if (node.position == kEnd) return node.cost;
    for (Vec offset : {Vec(-1, 0), Vec(1, 0), Vec(0, -1), Vec(0, 1)}) {
      const Vec neighbour = node.position + offset;
      if (!InBounds(neighbour)) continue;
      if (0 < input[neighbour] && input[neighbour] <= 1024) continue;
      frontier.Push({
          .cost = std::uint16_t(node.cost + 1),
          .guess = std::uint16_t(node.cost + GuessCost(neighbour)),
          .position = neighbour,
      });
    }
  }
  throw std::runtime_error("no solution");
}

class Walls {
 public:
  struct Node {
    Vec parent;
    std::uint16_t size : 13;
    // True if this node is transitively connected to the top right edge.
    std::uint16_t top_right : 1;
    // True if this node is transitively connected to the bottom left edge.
    std::uint16_t bottom_left: 1;
    // False if this node has not been added (and should thus be ignored).
    std::uint16_t seen : 1;
  };

  Walls() {
    for (int y = 0; y < 71; y++) {
      for (int x = 0; x < 71; x++) {
        nodes_[y][x].parent = Vec(x, y);
      }
    }
    for (int i = 0; i < 71; i++) {
      nodes_[i][0].bottom_left = true;
      nodes_[0][i].top_right = true;
      nodes_[i][70].top_right = true;
      nodes_[70][i].bottom_left = true;
    }
  }

  Node& Add(Vec position) {
    assert(!Get(position).seen);
    Get(position).seen = 1;
    static constexpr Vec kOffsets[] = {Vec(-1, -1), Vec(0, -1), Vec(1, -1),
                                       Vec(-1, 0),              Vec(1, 0),
                                       Vec(-1, 1), Vec(0, 1),   Vec(1, 1)};
    for (Vec offset : kOffsets) {
      const Vec neighbour = position + offset;
      if (!InBounds(neighbour) || !Get(neighbour).seen) continue;
      Merge(position, neighbour);
    }

    return Get(Find(position));
  }

 private:
  Node& Get(Vec position) {
    assert(InBounds(position));
    return nodes_[position.y][position.x];
  }

  Vec Root(Vec node) {
    Vec i = node;
    while (i != Get(i).parent) i = Get(i).parent;
    return i;
  }

  Vec Find(Vec node) {
    const Vec root = Root(node);
    Vec i = node;
    while (i != root) i = std::exchange(Get(i).parent, root);
    return i;
  }

  void Merge(Vec a, Vec b) {
    a = Find(a);
    b = Find(b);
    if (a == b) return;
    if (Get(a).size < Get(b).size) std::swap(a, b);
    Node& parent = Get(a);
    Node& child = Get(b);
    parent.size += child.size;
    parent.top_right |= child.top_right;
    parent.bottom_left |= child.bottom_left;
    child.parent = a;
  }

  Node nodes_[71][71] = {};
};

// The approach I'm using here is to search for an unbroken wall that connects
// the top/right with the bottom/left. If such a wall exists, the path from the
// top left corner to the bottom right is blocked. We can maintain the connected
// components with a union-find data structure.
Vec Part2(const Input& input) {
  Vec bytes[4000];
  int num_bytes = 0;
  for (int y = 0; y < 71; y++) {
    for (int x = 0; x < 71; x++) {
      if (input[x, y]) {
        if (num_bytes == 4000) throw std::runtime_error("too many bytes");
        bytes[num_bytes++] = Vec(x, y);
      }
    }
  }
  std::ranges::sort(std::span(bytes, num_bytes),
                    std::less<>(),
                    [&](Vec v) { return input[v]; });
  Walls walls;
  for (Vec byte : std::span(bytes, num_bytes)) {
    Walls::Node& wall = walls.Add(byte);
    if (wall.top_right && wall.bottom_left) return byte;
  }
  throw std::runtime_error("no byte obstructs the path");
}

}  // namespace

Task<void> Day18(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const Vec part2 = Part2(input);
  std::println("part1: {}\npart2: {},{}", part1, part2.x, part2.y);

  char result[64];
  const char* end =
      std::format_to(result, "{}\n{},{}\n", part1, part2.x, part2.y);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
