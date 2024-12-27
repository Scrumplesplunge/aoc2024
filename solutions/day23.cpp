#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

#include <algorithm>
#include <bitset>
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
  }

  // struct Node {
  //   std::uint16_t parent;
  //   std::uint8_t size;
  //   std::uint8_t num_ts;
  // };

  // Node* FindRoot(Node* node) {
  //   while (node != &nodes[node->parent]) {
  //     Node* next = &nodes[node->parent];
  //     node->parent = next->parent;
  //     node = next;
  //   }
  //   return node;
  // }

  // void Link(Node* a, Node* b) {
  //   a = FindRoot(a);
  //   b = FindRoot(b);
  //   if (a == b) return;
  //   if (a->size < b->size) {
  //     a->parent = b->parent;
  //     b->size += a->size;
  //     b->num_ts += a->num_ts;
  //   } else {
  //     b->parent = a->parent;
  //     a->size += b->size;
  //     a->num_ts += b->num_ts;
  //   }
  // }

  // Node nodes[26 * 26];

  std::vector<Node> nodes;
};

bool HasT(int id) { return id / 26 == 't' - 'a'; }

// 19384 too high
//   (enforced ijk ordering to avoid double counting)
// 3241 too high
//   (check that k connects back to i)
// 2666 too high
//   (fix bug where I was checking HasT on an index instead of an id)
// 2590 wrong answer
//   (I didn't read the question, I was counting any T, not just a starting T)
int Part1(const Input& input) {
  int total = 0;
  const int n = input.nodes.size();
  for (int i = 0; i < n; i++) {
    for (int j : input.nodes[i].neighbors) {
      if (j <= i) continue;
      for (int k : input.nodes[j].neighbors) {
        if (k <= j) continue;
        if (!std::ranges::contains(input.nodes[k].neighbors, i)) continue;
        // i is connected to j by construction.
        // j is connected to k by construction.
        // k is connected to i because of the check above.
        std::print("triple {},{},{}", input.nodes[i].id, input.nodes[j].id,
                     input.nodes[k].id);
        if (!HasT(input.nodes[i].id) && !HasT(input.nodes[j].id) &&
            !HasT(input.nodes[k].id)) {
          std::println(" has no T");
          continue;
        }
        std::println(" contains a T");
        total++;
      }
    }
  }
  return total;
}

int Part2(const Input&) {
  return 0;
}

}  // namespace

Task<void> Day23(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  const int part1 = Part1(input);
  const int part2 = Part2(input);

  char result[32];
  const char* end = std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
