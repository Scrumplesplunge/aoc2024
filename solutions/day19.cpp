#include <algorithm>
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
  if (i == input.npos) return false;
  word.value = input.substr(0, i);
  input.remove_prefix(i);
  return true;
}

struct Input {
  Task<void> Read(tcp::Socket& socket) {
    std::string_view input(co_await socket.Read(buffer));
    Word towel;
    if (!ScanPrefix(input, "{}", towel)) throw std::runtime_error("syntax");
    int num_strings = 0;
    strings[num_strings++] = towel.value;
    while (ScanPrefix(input, ", {}", towel)) {
      if (num_strings == kMaxStrings) throw std::runtime_error("too many");
      strings[num_strings++] = towel.value;
    }
    if (!ScanPrefix(input, "\n\n")) throw std::runtime_error("syntax");
    const int num_towels = num_strings;
    towels = std::span(strings).subspan(0, num_towels);
    Word pattern;
    while (ScanPrefix(input, "{}\n", pattern)) {
      if (num_strings == kMaxStrings) throw std::runtime_error("too many");
      strings[num_strings++] = pattern.value;
    }
    const int num_patterns = num_strings - num_towels;
    patterns = std::span(strings).subspan(num_towels, num_patterns);
    std::println("{} strings", num_strings);
  }

  char buffer[25000];
  static constexpr int kMaxStrings = 2048;
  std::string_view strings[kMaxStrings];
  std::span<const std::string_view> towels;
  std::span<const std::string_view> patterns;
};

std::uint64_t Arrangements(std::string_view pattern,
                 std::span<const std::string_view> towels) {
  std::uint64_t arrangements[100] = {};
  assert(pattern.size() < 100);
  arrangements[pattern.size()] = 1;
  for (int i = pattern.size(); i >= 0; i--) {
    const std::string_view suffix = pattern.substr(i);
    for (std::string_view towel : towels) {
      if (suffix.starts_with(towel)) {
        arrangements[i] += arrangements[i + towel.size()];
      }
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
  for (std::string_view pattern : input.patterns) {
    const std::uint64_t arrangements = Arrangements(pattern, input.towels);
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
