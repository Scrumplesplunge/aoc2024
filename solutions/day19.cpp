#include <algorithm>
#include <generator>
#include <print>

#include "../common/api.hpp"
#include "../common/coro.hpp"
#include "../common/scan.hpp"
#include "tcp.hpp"

namespace aoc2024 {
namespace {

struct Word { std::string_view value; };

bool ScanValue(std::string_view& input, Word& word) {
  const auto i = input.find_first_not_of("bgruw");
  if (i == 0 || i == input.npos) return false;
  word.value = input.substr(0, i);
  input.remove_prefix(i);
  return true;
}

int ToIndex(char c) {
  switch (c) {
    case 'b': return 0;
    case 'g': return 1;
    case 'r': return 2;
    case 'u': return 3;
    case 'w': return 4;
  }
  throw std::runtime_error("bad stripe");
}

class TowelTrie {
 public:
  void Add(std::string_view towel) {
    std::uint16_t i = 0;
    for (char c : towel) {
      std::uint16_t& branch = nodes_[i].next[ToIndex(c)];
      if (branch == 0) {
        if (num_nodes_ == kMaxNodes) throw std::runtime_error("too many nodes");
        branch = num_nodes_++;
      }
      i = branch;
    }
    nodes_[i].is_end = true;
  }

  std::generator<int> PrefixMatches(std::string_view string) const {
    std::uint16_t i = 0;
    for (int c = 0, n = string.size(); c < n; c++) {
      std::uint16_t branch = nodes_[i].next[ToIndex(string[c])];
      if (branch == 0) co_return;
      i = branch;
      if (nodes_[i].is_end) co_yield c + 1;
    }
  }

 private:
  static constexpr int kMaxNodes = 2048;
  struct Node {
    bool is_end;
    std::uint16_t next[5];
  };
  Node nodes_[kMaxNodes] = {};
  int num_nodes_ = 1;
};

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    std::string_view input(co_await socket.Read(buffer));
    Word towel;
    if (!ScanPrefix(input, "{}", towel)) throw std::runtime_error("syntax");
    towels.Add(towel.value);
    while (ScanPrefix(input, ", {}", towel)) towels.Add(towel.value);
    if (!ScanPrefix(input, "\n\n")) throw std::runtime_error("syntax");
    Word design;
    int num_designs = 0;
    while (ScanPrefix(input, "{}\n", design)) {
      if (num_designs == kMaxDesigns) throw std::runtime_error("too many");
      design_buffer[num_designs++] = design.value;
    }
    designs = std::span(design_buffer).subspan(0, num_designs);
  }

  char buffer[25000];

  TowelTrie towels;

  static constexpr int kMaxDesigns = 512;
  std::string_view design_buffer[kMaxDesigns];
  std::span<const std::string_view> designs;
};

std::uint64_t Arrangements(std::string_view pattern, const TowelTrie& towels) {
  std::uint64_t arrangements[100] = {};
  assert(pattern.size() < 100);
  arrangements[pattern.size()] = 1;
  for (int i = pattern.size(); i >= 0; i--) {
    const std::string_view suffix = pattern.substr(i);
    for (int prefix : towels.PrefixMatches(suffix)) {
      arrangements[i] += arrangements[i + prefix];
    }
  }
  return arrangements[0];
}

}  // namespace

Task<void> Day19(tcp::Socket& socket) {
  Input input;
  co_await input.Read(socket);

  int part1 = 0;
  std::uint64_t part2 = 0;
  for (std::string_view design : input.designs) {
    const std::uint64_t arrangements = Arrangements(design, input.towels);
    if (arrangements > 0) part1++;
    part2 += arrangements;
  }
  std::println("part1: {}\npart2: {}", part1, part2);

  char result[32];
  const char* end =
      std::format_to(result, "{}\n{}\n", part1, part2);
  co_await socket.Write(std::span<const char>(result, end - result));
}

}  // namespace aoc2024
