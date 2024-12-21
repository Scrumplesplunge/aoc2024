#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>

namespace aoc2024 {
namespace {

struct Input {
  struct Code {
    std::string_view text;
    int number;
  };

  Task<void> Read(tcp::Socket& socket) {
    std::string_view input(co_await socket.Read(buffer));
    if (input.size() != 25) throw std::runtime_error("bad input");
    for (int code = 0; code < 5; code++) {
      int number = 0;
      const std::string_view entry = input.substr(5 * code, 5);
      for (int i = 0; i < 3; i++) {
        if (!('0' <= entry[i] && entry[i] <= '9')) {
          throw std::runtime_error("bad code");
        }
        number = 10 * number + (entry[i] - '0');
      }
      if (entry[3] != 'A' || entry[4] != '\n') {
        throw std::runtime_error("bad code");
      }
      codes[code] = Code{.text = entry.substr(0, 4), .number = number};
    }
  }

  char buffer[26];
  Code codes[5];
};

struct Vec {
  friend inline bool operator==(const Vec& l, const Vec& r) = default;
  std::int8_t x, y;
};

Vec operator+(Vec l, Vec r) { return Vec(l.x + r.x, l.y + r.y); }

struct Grid {
  Vec operator[](char c) const {
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 3; x++) {
        if (buttons[y][x] == c) return Vec(x, y);
      }
    }
    throw std::logic_error("not found");
  }

  auto& operator[](this auto&& self, Vec position) {
    return self.buttons[position.y][position.x];
  }

  bool InBounds(Vec v) const {
    return 0 <= v.x && v.x < 3 && 0 <= v.y && v.y < 4 && (*this)[v] != '\0';
  }

  char buttons[4][3];
};

constexpr Grid kNumpad = {
    .buttons = {{'7',  '8', '9'},
                {'4',  '5', '6'},
                {'1',  '2', '3'},
                {'\0', '0', 'A'}},
};

constexpr Grid kArrows = {
    .buttons = {{'\0', '^', 'A'},
                {'<',  'v', '>'}},
};

class Frontier {
 public:
  struct Node {
    //     arrows       arrows       arrows       numpad
    // hand ---> robot_a ---> robot_b ---> robot_c ---> output
    Vec hand;
    Vec robot_a;
    Vec robot_b;
    char path[200];
    std::uint8_t cost;
    std::uint8_t digits_produced;
  };

  bool Empty() const { return size_ == 0; }

  Node Pop() {
    assert(!Empty());
    std::ranges::pop_heap(data_, data_ + size_, std::greater<>(), &Node::cost);
    return data_[--size_];
  }

  void Push(Node node) {
    assert(size_ < kMaxSize);
    data_[size_++] = node;
    std::ranges::push_heap(data_, data_ + size_, std::greater<>(),
                           &Node::cost);
  }

 private:
  static constexpr int kMaxSize = 750;
  Node data_[kMaxSize];
  int size_ = 0;
};

class VisitedSet {
 public:
  bool Insert(const Frontier::Node& node) {
    assert(kNumpad.InBounds(node.robot_b));
    const int robot_b = node.robot_b.y * 3 + node.robot_b.x;
    std::uint16_t& numpad = GetNumpad(node);
    if (numpad & (1 << robot_b)) return false;
    numpad |= 1 << robot_b;
    return true;
  }

 private:
  auto GetNumpad(this auto&& self, const Frontier::Node& node)
      -> decltype(self.data_[0][0][0])& {
    assert(kArrows.InBounds(node.hand));
    assert(kArrows.InBounds(node.robot_a));
    const int digits = node.digits_produced;
    const int hand = node.hand.y * 3 + node.hand.x;
    const int robot_a = node.robot_a.y * 3 + node.robot_a.x;
    return self.data_[digits][hand][robot_a];
  }

  // `data_[digits][hand][robot_a] & (1 << robot_b)` is set if we have
  // already seen this configuration.
  std::uint16_t data_[5][6][6] = {};
};

Vec Step(Vec position, char c) {
  switch (c) {
    case '<': return position + Vec(-1, 0);
    case '>': return position + Vec(1, 0);
    case '^': return position + Vec(0, -1);
    case 'v': return position + Vec(0, 1);
  }
  throw std::runtime_error("bad direction");
}

std::optional<Frontier::Node> TryAction(std::string_view target,
                                        Frontier::Node node, char c) {
  if (node.cost >= 200) {
    node.path[198] = node.path[199] = '.';
  } else {
    node.path[node.cost] = c;
  }
  node.cost++;
  if (c != 'A') {
    node.hand = Step(node.hand, c);
    if (!kArrows.InBounds(node.hand)) return std::nullopt;
    return node;
  }
  if (kArrows[node.hand] != 'A') {
    node.robot_a = Step(node.robot_a, kArrows[node.hand]);
    if (!kArrows.InBounds(node.robot_a)) return std::nullopt;
    return node;
  }
  if (kArrows[node.robot_a] != 'A') {
    node.robot_b = Step(node.robot_b, kArrows[node.robot_a]);
    if (!kNumpad.InBounds(node.robot_b)) return std::nullopt;
    return node;
  }
  if (kNumpad[node.robot_b] != target[node.digits_produced]) {
    return std::nullopt;
  }
  node.digits_produced++;
  return node;
}

int NumPresses(std::string_view code) {
  Frontier frontier;
  VisitedSet visited;
  frontier.Push({
      .hand = kArrows['A'],
      .robot_a = kArrows['A'],
      .robot_b = kNumpad['A'],
      .path = {},
      .cost = 0,
      .digits_produced = 0,
  });
  while (!frontier.Empty()) {
    const Frontier::Node node = frontier.Pop();
    if (!visited.Insert(node)) continue;
    if (node.digits_produced == code.size()) return node.cost;
    for (char action : {'A', '<', '>', '^', 'v'}) {
      const std::optional<Frontier::Node> next = TryAction(code, node, action);
      if (next) frontier.Push(*next);
    }
  }
  throw std::runtime_error("no valid sequence");
}

int Part1(const Input& input) {
  int total = 0;
  for (int i = 0; i < 5; i++) {
    const int num_presses = NumPresses(input.codes[i].text);
    total += input.codes[i].number * num_presses;
  }
  return total;
}

}  // namespace

Task<void> Day21(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const int part2 = 0;

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
