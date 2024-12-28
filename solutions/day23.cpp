#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <bitset>
#include <generator>
#include <map>
#include <print>

namespace aoc2024 {
namespace {

struct Computer { std::uint16_t id; };

bool IsLower(char c) { return 'a' <= c && c <= 'z'; }

bool ScanValue(std::string_view& input, Computer& c) {
  if (input.size() < 2 || !IsLower(input[0]) || !IsLower(input[1])) {
    return false;
  }
  c.id = 26 * (input[0] - 'a') + (input[1] - 'a');
  input.remove_prefix(2);
  return true;
}

struct Input {
  struct Node {
    int id;
    std::vector<std::uint16_t> neighbors;
  };

  Task<void> Read(tcp::Socket& socket) {
    char buffer[21000];
    std::string_view input(co_await socket.Read(buffer));

    int next_index = 0;
    std::map<int, int> indices;
    const auto get_index = [&](int id) {
      auto [i, is_new] = indices.try_emplace(id, next_index);
      if (is_new) {
        Node& node = nodes.emplace_back();
        node.id = id;
        next_index++;
      }
      return i->second;
    };

    Computer a, b;
    while (ScanPrefix(input, "{}-{}\n", a, b)) {
      const int i = get_index(a.id), j = get_index(b.id);
      nodes[i].neighbors.push_back(j);
      nodes[j].neighbors.push_back(i);
    }

    for (int i = 0; i < next_index; i++) {
      std::ranges::sort(nodes[i].neighbors);
    }
  }

  std::vector<Node> nodes;
};

bool HasT(int id) { return id / 26 == 't' - 'a'; }

std::generator<std::span<const int>> ConnectedGroups(
    std::vector<int>& stack, std::span<const Input::Node> nodes) {
  const int back = stack.back();
  co_yield stack;
  for (int i : nodes[back].neighbors) {
    if (i <= back) continue;
    if (!std::ranges::includes(nodes[i].neighbors, stack)) continue;
    stack.push_back(i);
    co_yield std::ranges::elements_of(ConnectedGroups(stack, nodes));
    stack.pop_back();
  }
}

std::generator<std::span<const int>> ConnectedGroups(
    std::span<const Input::Node> nodes) {
  const int n = nodes.size();
  for (int i = 0; i < n; i++) {
    std::vector<int> stack = {i};
    co_yield std::ranges::elements_of(ConnectedGroups(stack, nodes));
  }
}

std::string ToString(std::span<const int> group,
                     std::span<const Input::Node> nodes) {
  char result[128];
  char* out = result;
  bool first = true;
  for (int i : group) {
    if (first) {
      first = false;
    } else {
      *out++ = ',';
    }
    *out++ = nodes[i].id / 26 + 'a';
    *out++ = nodes[i].id % 26 + 'a';
  }
  return std::string(result, out - result);
}

int Part1(const Input& input) {
  int total = 0;
  for (std::span<const int> group : ConnectedGroups(input.nodes)) {
    if (group.size() != 3) continue;
    int num_ts = 0;
    for (int i : group) {
      if (HasT(input.nodes[i].id)) num_ts++;
    }
    if (num_ts > 0) total++;
  }
  return total;
}

std::string Part2(const Input& input) {
  std::vector<int> best;
  for (std::span<const int> group : ConnectedGroups(input.nodes)) {
    if (group.size() > best.size()) {
      best = std::vector(group.begin(), group.end());
    }
  }
  std::ranges::sort(best, std::less<>(),
                    [&](int i) { return input.nodes[i].id; });
  return ToString(best, input.nodes);
}

}  // namespace

Task<void> Day23(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const std::string part2 = Part2(input);

  char result[256];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
