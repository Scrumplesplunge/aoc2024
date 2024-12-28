#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <print>
#include <source_location>

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
    // Position of the robot hand in this layer.
    Vec position;
    // Number of digits pressed on the keypad controlled by this robot.
    std::uint16_t digits_produced;
    // Last instruction given to this robot (initially 'A').
    char previous;
    // Total cost (buttons pressed by hand) of instructions given to this robot.
    std::uint64_t cost;
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

int ActionIndex(char c) {
  static constexpr char kActions[] = "A<>v^";
  for (int i = 0; i < 5; i++) {
    if (kActions[i] == c) return i;
  }
  throw std::logic_error("bad action");
}

template <Grid kGrid>
class VisitedSet {
 public:
  bool Insert(const Frontier::Node& node) {
    assert(kGrid.InBounds(node.position));
    const int i = node.position.y * 3 + node.position.x;
    std::uint16_t& bitmap =
        data_[ActionIndex(node.previous)][node.digits_produced];
    if (bitmap & (1 << i)) return false;
    bitmap |= 1 << i;
    return true;
  }

 private:
  // `data_[previous][digits] & (1 << position)` is set if we have already seen
  // this configuration.
  std::uint16_t data_[6][5] = {};
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

class Costs {
 public:
  Costs() = default;
  Costs(std::uint64_t initial) {
    for (int a = 0; a < 5; a++) {
      for (int b = 0; b < 5; b++) {
        values_[a][b] = initial;
      }
    }
  }

  auto& operator[](this auto&& self, char previous, char next) {
    return self.values_[ActionIndex(previous)][ActionIndex(next)];
  }

 private:
  // `values_[a][b]` is the cost (in buttons pressed by hand) of moving the
  // robot hand in this layer from position `a` to position `b`.
  std::uint64_t values_[5][5];
};

template <Grid kGrid>
std::optional<Frontier::Node> TryAction(std::string_view target,
                                        Frontier::Node node, char c,
                                        const Costs& costs) {
  node.cost += costs[node.previous, c];
  node.previous = c;
  if (c != 'A') {
    node.position = Step(node.position, c);
    if (!kGrid.InBounds(node.position)) return std::nullopt;
    return node;
  }
  if (kGrid[node.position] != target[node.digits_produced]) {
    return std::nullopt;
  }
  node.digits_produced++;
  return node;
}

template <Grid kGrid>
std::uint64_t NumPresses(char initial, std::string_view target,
                         const Costs& costs) {
  Frontier frontier;
  VisitedSet<kGrid> visited;
  frontier.Push({
      .position = kGrid[initial],
      .digits_produced = 0,
      .previous = 'A',
      .cost = 0,
  });
  while (!frontier.Empty()) {
    const Frontier::Node node = frontier.Pop();
    if (!visited.Insert(node)) continue;
    if (node.digits_produced == target.size()) return node.cost;
    for (char action : {'A', '<', '>', '^', 'v'}) {
      const std::optional<Frontier::Node> next =
          TryAction<kGrid>(target, node, action, costs);
      if (next) frontier.Push(*next);
    }
  }
  throw std::runtime_error("no valid sequence");
}

template <int kNumInnerRobots>
std::uint64_t Solve(const Input& input) {
  Costs cost_buffer[2] = {Costs(1), Costs()};
  for (int i = 0; i < kNumInnerRobots; i++) {
    const Costs& previous_costs = cost_buffer[i % 2];
    Costs& next_costs = cost_buffer[(i + 1) % 2];
    static constexpr std::string_view kActions[] = {"A", "<", ">", "^", "v"};
    for (std::string_view previous : kActions) {
      for (std::string_view next : kActions) {
        next_costs[previous[0], next[0]] =
            NumPresses<kArrows>(previous[0], next, previous_costs);
      }
    }
  }
  const Costs& costs = cost_buffer[kNumInnerRobots % 2];
  std::uint64_t total = 0;
  for (int i = 0; i < 5; i++) {
    const std::uint64_t num_presses =
        NumPresses<kNumpad>('A', input.codes[i].text, costs);
    total += num_presses * input.codes[i].number;
  }
  return total;
}

}  // namespace

Task<void> Day21(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const std::uint64_t part1 = Solve<2>(input);
  const std::uint64_t part2 = Solve<25>(input);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
